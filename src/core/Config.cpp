#include "Config.h"
#include "Logger.h"
#include <fstream>

namespace a320 {

std::optional<json> Config::loadOrDefault(const char* relPath)
{
    std::ifstream f(relPath);
    if (!f.is_open()) return std::nullopt;

    try {
        return json::parse(f);
    } catch (const json::parse_error& e) {
        LOG_ERROR("Config parse error in {}: {}", relPath, e.what());
        return std::nullopt;
    }
}

} // namespace a320
