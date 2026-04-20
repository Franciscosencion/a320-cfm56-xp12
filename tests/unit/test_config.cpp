#include <catch2/catch_test_macros.hpp>
#include "core/Config.h"
#include <fstream>
#include <filesystem>

using namespace a320;

TEST_CASE("Config returns nullopt for missing file", "[core][config]")
{
    auto result = Config::loadOrDefault("/nonexistent/path/config.json");
    REQUIRE(!result.has_value());
}

TEST_CASE("Config parses valid JSON", "[core][config]")
{
    const char* tmpPath = "test_config_tmp.json";
    {
        std::ofstream f(tmpPath);
        f << R"({"network":{"udp_port":45010},"debug":{"log_level":"info"}})";
    }

    auto result = Config::loadOrDefault(tmpPath);
    REQUIRE(result.has_value());
    REQUIRE(Config::get<int>((*result)["network"], "udp_port", 0) == 45010);
    REQUIRE(Config::get<std::string>((*result)["debug"], "log_level", "") == "info");

    std::filesystem::remove(tmpPath);
}
