module;

#include <iostream>
#include <string>

export module faelight.log;

#define LOG_LEVELS \
        X(JOKE,    joke,  95, 4) \
        X(DEBUG,   debug, 94, 3) \
        X(INFO,    info,  92, 2) \
        X(WARNING, warn,  93, 1) \
        X(ERROR,   err,   91, 0) \

export namespace FL::Log {

    enum class eLogLevel : int {
        #define X(var_name, func_name, color, log_level) var_name = log_level,
            LOG_LEVELS
        #undef X
    };

    class Config {
        inline static eLogLevel log_level = eLogLevel::INFO;
    public:
        static void      setLogLevel(eLogLevel level) {log_level = level;}
        static eLogLevel getLogLevel() {return log_level;}
    };


    #define X(var_name, func_name, color, log_level) const std::string var_name = std::format("[{}]", #var_name);
        LOG_LEVELS
    #undef X

    #define X(var_name, func_name, color, log_level) \
    template <typename... Args> void func_name(const std::format_string<Args...>& fmt, Args&&... args) { \
        if(int(Config::getLogLevel()) < log_level) return;\
        std::cout << "\033["<<#color<<"m"<<var_name<<"\033[0m " << std::format(fmt, std::forward<Args>(args)...) << "\n";\
    }
        LOG_LEVELS
    #undef X




};