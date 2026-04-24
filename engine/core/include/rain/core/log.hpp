#pragma once

#include<string_view>

namespace rain{
    enum class log_level{
        info,
        warning,
        error
    }

    void log_message(log_level level,std::string_view message);
    void log_info(std::string_view message);
    void log_warning(std::string_view message);
    void log_error(std::string_view message);
}