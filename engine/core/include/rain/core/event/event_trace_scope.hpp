#pragma once

#include <rain/core/event/event_system.hpp>

namespace rain{
    class scoped_event_trace{
    public:
        scoped_event_trace(event_system& events,bool enabled):events_(&events),previous_enabled_(events.trace_log().is_enabled()){
            events_.trace_log().set_enabled(enabled);
        }

        scoped_event_trace(const scoped_event_trace&)=delete;
        scoped_event_trace& operator=(const scoped_event_trace&)=delete;

        scoped_event_trace(scoped_event_trace&&other)noexcept:events_(other.events_),previous_enabled_(other.previous_enabled_){
            other.events_ = nullptr;
        }

        scoped_event_trace&operator=(const scoped_event_trace&&other)noexcept{
            if(this==&other){
                return *this;
            }

            restore();

            events_ = other.events_;

            previous_enabled_ = other.previous_enabled_;
            other.events_ = nullptr;

            return*this;
        }

        ~scoped_event_trace(){
            restore();
        }

    private:
        void restore(){
            if(events_==nullptr){
                return;
            }

            events_->trace_log().set_enabled(previous_enabled_);
            events_=nullptr;
        }

    private:
        event_system* events_ = nullptr;
        bool previous_enabled_ = true;
    }

}