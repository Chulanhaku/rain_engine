#pragma once

#include<rain/core/event/event_system.hpp>
#include<rain/core/types.hpp>
#include<rain/runtime/world.hpp>
#include<rain/runtime/fixed_step.hpp>

#include<algorithm>
#include<cmath>
#include<string>
#include<vector>

namespace rain{

    enum class system_tick_type :u8 {
        variable,
        fixed
    };

    enum class system_phase :u8 {
        pre_update,
        input,
        gameplay,
        movement,
        physics,
        post_physics,

        animation,
        render_prepare,

        post_update
    };

    [[nodiscard]] inline const char* to_string(system_phase phase) {
        switch (phase)
        {
        case system_phase::pre_update:
            return "pre_update";
        case system_phase::input:
            return "input";
        case system_phase::gameplay:
            return "gameplay";
        case system_phase::movement:
            return "movement";
        case system_phase::physics:
            return "physics";
        case system_phase::post_physics:
            return "post_physics";
        case system_phase::animation:
            return "animation";
        case system_phase::render_prepare:
            return "render_prepare";
        case system_phase::post_update:
            return "post_update";
        }

        return "unknown";
    }

    struct system_context{
        world* target_world = nullptr;
        event_system* events = nullptr;
        const entity_query_desc* entity_query = nullptr;
        system_phase phase = system_phase::pre_update;

        system_tick_type tick_type = system_tick_type::variable;

        f32 delta_seconds = 0.0f;
        
        f32 interpolation_alpha = 0.0f;
        u64 frame_index=0;
        u64 fixed_tick_index = 0;
    };

    struct system_debug_info {
        std::string system_name;
        std::string owner_name;

        system_phase phase = system_phase::gameplay;

        i32 priority = 0;
        bool enabled = true;

        usize required_component_count = 0;
        usize required_all_tag_count = 0;
        usize required_any_tag_count = 0;
        usize rejected_tag_count = 0;
    };

    struct system_run_info {
        f32 delta_seconds = 0.0f;
        f32 interpolation_alpha = 0.0f;
        u64 frame_index = 0;
        u64 fixed_tick_index = 0;

        system_tick_type tick_type = system_tick_type::variable;
    };

    class system_scheduler{
    public:
        using system_function = void(*)(system_context&context,void* user_data);

        struct system_desc{
            std::string system_name;
            std::string owner_name;
            system_phase phase = system_phase::gameplay;

            i32 priority = 0;
            bool enabled = true;

            entity_query_desc entity_query;

            system_function function = nullptr;
            void* user_data = nullptr;
        };

        void add_system(system_desc desc){
            systems_.push_back(std::move(desc));
            order_dirty_=true;
        }

        void run_phase(system_phase phase,world& target_world,event_system& events,const system_run_info& run_info) {
            rebuild_order_if_needed();
            events.begin_frame(run_info.frame_index);
            for (const u32 index : sorted_system_indices_) {
                auto& system = systems_[index];
                if (system.phase!=phase || !system.enabled || !system.function) continue;
                system_context context{
                    .target_world=&target_world,.events=&events,.entity_query=&system.entity_query,
                    .phase=system.phase,.tick_type=run_info.tick_type,.delta_seconds=run_info.delta_seconds,
                    .interpolation_alpha=run_info.interpolation_alpha,.frame_index=run_info.frame_index,
                    .fixed_tick_index=run_info.fixed_tick_index};
                system.function(context,system.user_data);
            }
        }

        // Production application and headless samples share the same fixed-step runner.
        // Excess whole steps beyond the budget are discarded; the fractional remainder
        // is retained for interpolation, preventing an unbounded catch-up backlog.
        void run_frame(world& target_world,event_system& events,f32 frame_delta,u64 frame_index,
            const fixed_step_settings& settings,fixed_step_state& state) {
            const f32 delta = std::isfinite(frame_delta) && frame_delta>0 ? frame_delta : 0.0f;
            if (!std::isfinite(state.accumulator) || state.accumulator<0) state.accumulator=0;
            if (!std::isfinite(state.interpolation_alpha)) state.interpolation_alpha=0;
            system_run_info variable{.delta_seconds=delta,.interpolation_alpha=state.interpolation_alpha,
                .frame_index=frame_index,.fixed_tick_index=state.tick_index,.tick_type=system_tick_type::variable};
            for (auto phase : {system_phase::pre_update,system_phase::input,system_phase::gameplay,system_phase::movement})
                run_phase(phase,target_world,events,variable);

            if (std::isfinite(settings.delta_seconds) && settings.delta_seconds>0 &&
                std::isfinite(settings.max_frame_delta) && settings.max_frame_delta>0 && settings.max_substeps>0) {
                const double step = settings.delta_seconds;
                // Settings/frame deltas enter as float; tolerate their boundary rounding.
                const double epsilon = step * 0.00001;
                state.accumulator += std::min(delta,settings.max_frame_delta);
                u32 count=0;
                while (state.accumulator+epsilon>=step && count<settings.max_substeps) {
                    const system_run_info fixed{.delta_seconds=settings.delta_seconds,.interpolation_alpha=0,
                        .frame_index=frame_index,.fixed_tick_index=state.tick_index,.tick_type=system_tick_type::fixed};
                    run_phase(system_phase::physics,target_world,events,fixed);
                    run_phase(system_phase::post_physics,target_world,events,fixed);
                    state.accumulator = std::max(state.accumulator-step,0.0);
                    ++state.tick_index;
                    ++count;
                }
                if (state.accumulator+epsilon>=step) {
                    state.accumulator = std::fmod(state.accumulator,step);
                    if (state.accumulator+epsilon>=step) state.accumulator=0;
                }
                state.interpolation_alpha = std::clamp(static_cast<f32>(state.accumulator/step),
                    0.0f,std::nextafter(1.0f,0.0f));
            } else {
                state.accumulator=0;
                state.interpolation_alpha=0;
            }
            variable.interpolation_alpha=state.interpolation_alpha;
            variable.fixed_tick_index=state.tick_index;
            for (auto phase : {system_phase::animation,system_phase::post_update,system_phase::render_prepare})
                run_phase(phase,target_world,events,variable);
        }

        [[nodiscard]] std::vector<system_debug_info>debug_infos()const {
            std::vector<system_debug_info>result;
            result.reserve(systems_.size());

            for (const system_desc& system : systems_)
            {
                result.push_back(system_debug_info{
                    .system_name = system.system_name,
                    .owner_name = system.owner_name,
                    .phase = system.phase,
                    .priority = system.priority,
                    .enabled = system.enabled,
                    .required_component_count = system.entity_query.required_components.size(),
                    .required_all_tag_count = system.entity_query.required_tags.all_tags().size(),
                    .required_any_tag_count = system.entity_query.required_tags.any_tags().size(),
                    .rejected_tag_count = system.entity_query.required_tags.none_tags().size()
                });
            }

            return result;
        }

        void run_all(world& target_world,event_system&events,f32 delta_seconds,u64 frame_index){
            rebuild_order_if_needed();

            events.begin_frame(frame_index);

            for(const u32 system_index: sorted_system_indices_){
                system_desc& system = systems_[system_index];
                if(!system.enabled||system.function==nullptr)continue;

                system_context context{
                    .target_world = &target_world,
                    .events = &events,
                    .entity_query = &system.entity_query,
                    .phase = system.phase,
                    .delta_seconds = delta_seconds,
                    .frame_index = frame_index
                };

                system.function(context,system.user_data);
            }
        }

        void set_enabled(const std::string& system_name,bool enabled){
            for(system_desc&system:systems_){
                if(system.system_name==system_name){
                    system.enabled  = enabled;
                    return;
                }
            }
        }

        [[nodiscard]]usize system_count()const{
            return systems_.size();
        }

        [[nodiscard]]const std::vector<system_desc>& systems()const{
            return systems_;
        }

    private:
        void rebuild_order_if_needed(){
            if(!order_dirty_)return;

            sorted_system_indices_.clear();
            sorted_system_indices_.reserve(systems_.size());

            for(usize i =0;i<systems_.size();i++){
                sorted_system_indices_.push_back(static_cast<u32>(i));
            }

            std::sort(sorted_system_indices_.begin(),sorted_system_indices_.end(),[this](u32 lhs_index,u32 rhs_index){
                const system_desc& lhs = systems_[lhs_index];
                const system_desc& rhs = systems_[rhs_index];

                if (phase_order(lhs.phase) != phase_order(rhs.phase)) {
                    return phase_order(lhs.phase) < phase_order(rhs.phase);
                }

                if(lhs.priority!=rhs.priority){
                    return lhs.priority>rhs.priority;
                }

                return lhs.system_name<rhs.system_name;
            });

            order_dirty_ = false;
        }


        [[nodiscard]] static u32 phase_order(system_phase phase){
            switch (phase)
            {
            case system_phase::pre_update:
                return 0;
            case system_phase::input:
                return 10;
            case system_phase::gameplay:
                return 20;
            case system_phase::movement:
                return 30;
            case system_phase::physics:
                return 40;
            case system_phase::post_physics:
                return 50;
            case system_phase::animation:
                return 60;
            case system_phase::post_update:
                return 70;
            case system_phase::render_prepare:
                return 80;
            }

            return 999;
        }
    private:
        std::vector<system_desc>systems_;
        std::vector<u32>sorted_system_indices_;
        bool order_dirty_ = true;
    };
}