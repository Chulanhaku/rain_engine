#include<cstdio>

#include<rain/core/log.hpp>

namespace rain {
    void log_message(log_level level,std::string_view message){
        const char* prefix = "[rain][info] ";

        switch(level){
            case log_level::info:
                prefix =  "[rain][info] ";
                break;
            case log_level::warning:
                prefix =  "[rain][warning] ";
                break;
            case log_level::error:
                prefix =  "[rain][error] ";
                break;
        }

        std::printf("%s%.*s\n",prefix,static_cast<int>(message.size()),message.data());
    }

    void log_info(std::string_view message){
        log_message(log_level::info,message);
    }

    void log_error(std::string_view message){
        log_message(log_level::error,message);
    }

    void log_warning(std::string_view message){
        log_message(log_level::warning,message);
    }
}