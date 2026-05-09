#pragma once

#include<rain/core/event/event_system.hpp>
#include<rain/core/types.hpp>
#include<rain/runtime/world.hpp>

#include<algorithm>
#include<string>
#include<vector>

namespace rain{
    struct system_context{
        world* target_world = nullptr;
        event_system* events = nullptr;
        f32 delta_seconds = 0.0f;
        u64 frame_index=0;
    };

    class system_scheduler{
    public:
        using system_function = void(*)(system_context&context,void* user_data);

        struct system_desc{
            std::string system_name;
            std::string owner_name;
            std::string phase_name = "update";

            i32 priority = 0;
            bool enabled = true;

            system_function function = nullptr;
            void* user_data = nullptr;
        };

        void add_system(system_desc desc){
            systems_.push_back(std::move(desc));
            order_dirty_=true;
        }

        void run(world& target_world,event_system&events,f32 delta_seconds,u64 frame_index){
            rebuild_order_if_needed();

            events.begin_frame(frame_index);

            system_context context{
                .target_world = &target_world,
                .events = &events,
                .delta_seconds = delta_seconds,
                .frame_index = frame_index
            };

            for(const u32 system_index: sorted_system_indices_){
                system_desc& system = systems_[system_index];
                if(!system.enabled||system.function==nullptr)continue;

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

                if(lhs.phase_name!=rhs.phase_name){
                    return lhs.phase_name<rhs.phase_name;
                }

                if(lhs.priority!=rhs.priority){
                    return lhs.priority>rhs.priority;
                }

                return lhs.system_name<rhs.system_name;
            });

            order_dirty_ = false;
        }

    private:
        std::vector<system_desc>systems_;
        std::vector<u32>sorted_system_indices_;
        bool order_dirty_ = true;
    };
}