#pragma once
#include <ctime>
#include <cstdarg>
#include <cstdio>

#define INFO    "INF"
#define ERROR   "ERR"

inline void log_message_va(const char* module, const char* level, const char* fmt, va_list args) {
    FILE* out = (level[0] == 'E') ? stderr : stdout;
    char ts[32];
    std::time_t t = std::time(nullptr);
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    std::fprintf(out, "[%s] [%s] [%s] ", ts, level, module);
    std::vfprintf(out, fmt, args);
    std::fprintf(out, "\n");
}

inline void log_message(const char* module, const char* level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message_va(module, level, fmt, args);
    va_end(args);
}

#define LOG_INFO(module, fmt, ...)  log_message(module, INFO, fmt, ##__VA_ARGS__)
#define LOG_ERROR(module, fmt, ...) log_message(module, ERROR, fmt, ##__VA_ARGS__)

class Loggable {
public:
    virtual const char* module_name() const = 0;

    void log_info(const char* fmt, ...) const {
        va_list args;
        va_start(args, fmt);
        log_message_va(module_name(), INFO, fmt, args);
        va_end(args);
    }

    void log_error(const char* fmt, ...) const {
        va_list args;
        va_start(args, fmt);
        log_message_va(module_name(), ERROR, fmt, args);
        va_end(args);
    }
};
