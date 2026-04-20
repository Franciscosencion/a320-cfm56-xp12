#pragma once
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <string>

namespace a320 {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static void init(const char* filename);
    static void shutdown();
    static void log(LogLevel level, std::string msg);

    template<typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Debug, fmt::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Info,  fmt::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Warn,  fmt::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Error, fmt::format(fmt, std::forward<Args>(args)...));
    }
};

} // namespace a320

#define LOG_DEBUG(...) ::a320::Logger::debug(__VA_ARGS__)
#define LOG_INFO(...)  ::a320::Logger::info(__VA_ARGS__)
#define LOG_WARN(...)  ::a320::Logger::warn(__VA_ARGS__)
#define LOG_ERROR(...) ::a320::Logger::error(__VA_ARGS__)
