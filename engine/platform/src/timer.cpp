#include<rain/platform/timer.hpp>

namespace rain{
    frame_timer::frame_timer():last_time_(clock_type::now()){
    }

    float frame_timer::tick(){
        const auto current_time = clock_type::now();
        const std::chrono::duration<float> delta_time = current_time - last_time_;
        last_time_ = current_time;
        return delta_time.count();
    }




}