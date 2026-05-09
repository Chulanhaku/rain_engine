#pragma once

#include<rain/core/assert.hpp>
#include<rain/core/container/rain_hash_map.hpp>
#include<rain/core/event/event_context.hpp>
#include<rain/core/event/event_trace.hpp>
#include<rain/core/types.hpp>
#include<rain/core/type_id.hpp>

#include<algorithm>
#include<functional>
#include<memory>
#include<string>
#include<typeinfo>
#include<utility>
#include<vector>

namespace rain{
    struct event_register_desc{
        std::string event_name;
        std::string category;
        bool allow_immediate_dispatch = true;
        bool allow_queued_dispatch = true;
        bool trace_enabled = true; 
    };

    template<typename event_type>
    struct event_listener_desc{
        std::string listener_name;
        std::string owner_name;
        i32 priority = 0;
        bool enabled = true;

        std::function<void(const event_type& event,const event_context& cotext)>callback;
    };

    struct event_type_debug_info{
        type_id id;
        std::string event_name;
        std::string category;
        usize event_size = 0;
        usize queued_count = 0;
        usize listener_count = 0;
        bool allow_immediate_dispatch = true;
        bool allow_queued_dispatch = true;
        bool trace_enabled = true;
    };

    struct event_listener_debug_info{
        std::string listener_name;
        std::string owner_name;
        i32 priority = 0;
        bool alive = false;
        bool enabled = false;
        u32 generation = 0;
    };

    struct event_listener_table_entry{
        type_id event_type;
        std::string event_name;
        std::string category;

        std::string listener_name;
        std::string owner_name;

        i32 priority=0;
        bool alive = false;
        bool enabled = false;
        u32 generation = 0;

    }

    class event_system{
    public:
        template<typename event_type>
        void register_event(event_register_desc desc){
            rain_assert(!desc.event_name.empty());

            event_channel<event_type>& channel = get_or_create_channel<event_type>();

            channel.info.event_name = std::move(desc.event_name);
            channel.info.category = std::move(desc.category);
            channel.info.event_size = sizeof(event_type);
            channel.info.allow_immediate_dispatch = desc.allow_immediate_dispatch;
            channel.info.allow_queued_dispatch = desc.allow_queued_dispatch;
            channel.info.trace_enabled = desc.trace_enabled;
        }

        template<typename event_type>
        event_listener_handle add_listener(event_listener_desc<event_type>desc){
            rain_assert(!desc.listener_name.empty());
            rain_assert(!desc.owner_name.empty());
            rain_assert(desc.callback!=nullptr);

            event_channel<event_type>& channel = get_or_create_channel<event_type>();

            const u32 index  =static_cast<u32>(channel.listeners.size());

            typename event_channel<event_type>::listener_slot slot;
            slot.listener_name = std::move(desc.listener_name);
            slot.owner_name = std::move(desc.owner_name);
            slot.priority = desc.priority;
            slot.enabled = desc.enabled;
            slot.alive = true;
            slot.generation = 0;
            slot.callback = std::move(desc.callback);
            slot.order = channel.next_listener_order++;

            return event_listener_handle{
                .event_type = get_type_id<event_type>(),
                .index = index,
                .generation = 0
            };
        }

        template<typename event_type>
        void remove_listener(event_listener_handle handle){
            if(handle.event_type != get_type_id<event_type>()){
                return
            }

            event_channel<event_type>*channel = try_get_channel<event_type>();

            if(channel == nullptr)return;

            if(handle.index>=channel->listeners.size())return;

            auto &slot = channel->listeners[handle.index];

            if(!slot.alive||slot.generation!=handle.generation)return;

            slot.alive =false;
            slot.enabled = false;
            ++slot.generation ;
            channel->listener_order_dirty = true;
        }

        template<typename event_type>
        void set_listener_enabled(event_listener_handle handle,bool enabled){
            if(handle.event_type!= get_type_id<event_type>())return;

            event_channel<event_type>*channel = try_get_channel<event_type>();
            
            if(channel == nullptr)return;

            if(handle.index>= channel->listeners.size())return;

            auto& slot = channel->listeners[handle.index];

            if(!slot.alive||slot.generation!=handle.generation)return;

            slot.enabled = enabled;
        }

        template<typename event_type>
        void publish_now(const event_type& event,event_emit_desc desc){
            event_channel<event_type>& channel = get_or_create_channel<event_type>();

            rain_assert(channel.info.allow_immediate_dispatch);

            const event_context context = make_event_context(event_dispatch_mode::immediate,channel.info.event_name,desc,next_sequence(),frame_index_);

            channel.dispatch_now(event,context,trace_log_);
        }

        template<typename event_type>
        void enqueue(const event_type& event,event_emit_desc desc){
            event_channel<event_type>& channel = get_or_create_channel<event_type>();

            rain_assert(channel.info.allow_queued_dispatch);

            const event_context context = make_event_context(event_dispatch_mode::queued,channel.info.event_name,desc,next_sequence(),frame_index_);

            channel.queued_events.push_back(typename event_channel<event_type>::queued_event{.event = event,.context = context})
        }

        template<typename event_type>
        void dispatch_queued(){
            event_channel<event_type>* channel = try_get_channel<event_type>();

            if(channel == nullptr)return;

            channel->dispatch_queued(trace_log_);
        }

        void dispatch_all_queued(){
            for(std::unique_ptr<event_channel_base>& channel:channels_){
                channel->dispatch_queued_base(trace_log_);
            }
        }

        void clear_all_queued(){
            for(std::unique_ptr<event_channel_base>& channel:channels_){
                channel->clear_queued_base();
            }
        }

        void begin_frame(u64 frame_index){
            frame_index_ = frame_index;
        }

        [[nodiscard]] event_trace_log& trace_log(){
            return tace_log_;
        }

        [[nodiscard]]std::vector<event_listener_table_entry>get_all_listener_debug_infos()const{
            std::vector<event_listener_table_entry> result;

            for(const std::unique_ptr<event_channel_base>& channel : channels_){
                channel->collect_listener_debug_info_base(result);
            }

            return result;
        }

        [[nodiscard]] const event_trace_log& trace_log()const{
            return trace_log_;
        }

        [[nodiscard]]std::vector<event_type_debug_info>get_event_debug_infos()const{
            std::vector<event_type_debug_info>infos;
            infos.reserve(channels_.size());

            for(const std::unique_ptr<event_channel_base>& channel:channels_){
                infos.push_back(channel->get_debug_info_base());
            }

            return infos;
        }

        template<typename event_type>
        [[nodiscard]]std::vector<event_listener_debug_info>get_listener_debug_infos()const{
            const event_channel<event_type>*try_get_channel<event_type>();

            if(channel==nullptr){
                return{};
            }

            return channel->get_listener_debug_infos();
        }

    private:
        struct event_channel_base{
            virtual~event_channel_base()=default;
            virtual void collect_listener_debug_info_base(std::vector<event_listener_table_entry>&output)const = 0;
            virtual void dispatch_queued_base(event_trace_log&trace_log)=0;
            virtual void clear_queued_base()=0;
            virtual event_type_debug_info get_debug_info_base()const =0;
        };

        template<typename event_type>
        struct event_channel final:event_channel_base{
            struct listener_slot{
                std::string listener_name;
                std::string owner_name;

                i32 priority=0;
                u32 generation=0;
                u32 order=0;

                bool alive =false;
                bool enabled =false;

                std::function<void(const event_type&event,const event_context& context)>callback;
            };

            struct queued_event{
                event_type event;
                event_context context;
            };

            explicit event_channel(type_id type){
                info.id = type;
                info.event_name = typeid(event_type).name();
                info.category = "unregistered";
                info.event_size = sizeof(event_type);
            }

            void dispatch_now(const event_type& event,const event_context& context,event_tarce_log& trace_log){
                rebuild_listener_order_if_needed();
                for(const u32 listener_index:sorted_listener_indices){
                    listener_slot& slot = listeners[listener_index];

                    if(!slot.alive||!slot.enabled||slot.callback==nullptr)continue;

                    if(info.trace_enabled){
                        trace_log.push(event_trace_entry{.context = context , .listener_name = slot.listener_name,.listener_owner = slot.owner_name,.listener_priority = slot.priority});
                    }

                    slot.callback(event,context);
                }
            }


            void dispatch_queued(event_trace_log& trace_log)override{
                const std::vector<queued_event> events_to_dispatch = std::move(queued_events);
                queued_events.clear();

                for(const queued_event&queued :events_tp_dispatch){
                    dispatch_now(queued.evnet,queued.context,tracce_log);
                }
            }


            void dispatch_queued_base(event_trace_log&trace_log)override{
                distpatch_queued(trace_log);
            }

            void clear_queued_base()override{
                queued_events.clear();
            }

            void collect_listener_debug_info_base(std::vector<event_listener_table_entry>&output)const override{
                for(const listener_slot&slot:listeners){
                    output.push_back(event_listener_table_entry{
                        .event_type = info.id,
                        .event_name = info.event_name,
                        .category = info.category,
                        .listener_name = slot.listener_name,
                        .owner_name = slot.owner_name,
                        .priority = slot.priority,
                        .alive = slot.alive,
                        .enabled = slot.enabled,
                        .generation = slot.generation
                    });

                }
            }

            event_type_debug_info get_debug_info_base()const override{
                event_type_debug_info result = info;
                result.queued_count = queued_events.size();
                usize alive_listener_count = 0;

                for(const listener_slot&slot:listeners){
                    if(slot.alive){
                        ++alive_listener_count;
                    }
                }

                result.listener_count = alive_listener_count;

                return result;
            }

            [[nodiscard]] std::vector<event_listener_debug_info> get_listener_debug_infos()const{
                std::vector<event_listener_debug_info>result;
                result.reserve(listeners.size());

                for(const listener_slot& solt:listeners ){
                    result.push_back(event_listener_debug_info{.listener_name = slot.listener_name,.owner_name=slot.owner_name,.priority = slot.priority,.alive = slot.alive,.enabled = slot.enabled,.generation = slot.generation});
                }
                return result;
            }

            void rebuild_listener_order_if_needed(){
                if(!listener_order_dirty)return;

                sorted_listener_indices.clear();
                sorted_listener_indices.reserve(listener.size());

                for(usize i =0;i<listeners.size();++i){
                    if(listeners[i].alive){
                        sorted_listener_indices.push_back(static_cast<u32>(i));
                    }
                }

                std::sort(sorted_listener_indices.begin(),sorted_listener_indices.end(),[this](u32 lhs_index,u32 rhs_index){
                    const listener_slot& lhs =listener[lhs_index];
                    const listener_slot&rhs  = listener[rhs_index];
                    if(lhs.priority != rhs.priority){
                        return lhs.priority>rhs.priority;
                    }

                    return lhs.order<rhs.order;
                });
                listener_order_dirty = false;
            }

            event_type_debug_info info;

            std::vector<listener_slot> listeners;
            std::vector<u32>sorted_listener_indices;
            std::vector<queued_event> queued_events;

            bool listener_order_dirty = true;
            u32 next_listener_order = 0;
        };

        template<typename event_type>
        [[nodiscard]] event_channel<event_type>* try_get_channel(){
            const type_id id = get_type_id<event_type>();
            const usize* channel_index = type_to_channel_index_.find(id);

            if(channel_index==nullptr){
                return nullptr;
            }

            return static_cast<const event_channel<event_type>*>(channels_[*channel_index].get());
        }

        template<typename event_type>
        [[nodiscard]] event_channel<event_type>&get_or_create_channel(){
            const type_id id = get_type_id<event_type>();
            const usize* channel_index = type_to_channel_index_.find(id);

            if(channel_index!=nullptr)return *static_cast<event_channel<event_type>*>(channels_[*channel_index].get());

            const usize new_channel_index = channels_.size();

            auto channel = std::make_unique<event_channel<event_type>>(id);
            event_channel<event_type>* raw_channel = channel.get();

            channels_.push_back(std::move(channel));
            type_to_channel_index_.insert(id,new_channel_index);

            return *raw_channel;
        }

        [[nodiscard]]u64 next_sequence(){
            const u64 result = next_sequence_;
            ++next_sequence_;
            return result;
        }
    private:
        rain_hash_map<type_id,usize>type_to_channel_index_;
        std::vector<std::unique_ptr<event_channel_base>>channels_;

        event_trace_log trace_log_;

        u64 next_sequence_ = 0;
        u64 frame_index_ = 0;
    }

}