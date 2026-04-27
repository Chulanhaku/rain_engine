#pragma once
#include <chrono>

namespace rain{
    class frame_timer{
    public:
        frame_timer();

        float tick();

    private:
        using clock_type = std::chrono::steady_clock;

        clock_type::time_point last_time_;
    };


}