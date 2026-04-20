#include "Logger.h"
#ifndef UNIT_TESTING
#  include <XPLM/XPLMUtilities.h>
#endif
#include <fmt/chrono.h>

#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>

namespace a320 {

static std::ofstream  g_file;
static std::mutex     g_mutex;

static const char* levelStr(LogLevel l) {
    switch (l) {
        case LogLevel::Debug: return "DBG";
        case LogLevel::Info:  return "INF";
        case LogLevel::Warn:  return "WRN";
        case LogLevel::Error: return "ERR";
    }
    return "???";
}

void Logger::init(const char* filename)
{
    std::lock_guard lock(g_mutex);
    g_file.open(filename, std::ios::out | std::ios::trunc);
}

void Logger::shutdown()
{
    std::lock_guard lock(g_mutex);
    if (g_file.is_open()) g_file.close();
}

void Logger::log(LogLevel level, std::string msg)
{
    auto now = std::chrono::system_clock::now();
    auto line = fmt::format("[{:%H:%M:%S}][{}] {}\n", now, levelStr(level), msg);

    {
        std::lock_guard lock(g_mutex);
        if (g_file.is_open()) {
            g_file << line;
            g_file.flush();
        }
    }

#ifndef UNIT_TESTING
    // Also send to X-Plane's log.txt (safe to call from any thread)
    XPLMDebugString(line.c_str());
#endif
}

} // namespace a320
