#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace a320 {

using json = nlohmann::json;

class Config {
public:
    // Loads JSON from an X-Plane preferences path. Returns nullopt if file not found.
    static std::optional<json> loadOrDefault(const char* relPath);

    // Convenience getter with fallback
    template<typename T>
    static T get(const json& cfg, const char* key, T defaultVal) {
        if (cfg.contains(key)) return cfg.at(key).get<T>();
        return defaultVal;
    }
};

} // namespace a320
