#include <catch2/catch_test_macros.hpp>
#include "core/EventBus.h"

using namespace a320;

struct TemperatureChanged { float degC; };
struct EngineStarted      { int   engineIndex; };

TEST_CASE("EventBus delivers events to subscribers", "[core][eventbus]")
{
    EventBus bus;

    float receivedTemp = -1.f;
    bus.subscribe<TemperatureChanged>([&](const TemperatureChanged& e) {
        receivedTemp = e.degC;
    });

    bus.publish(TemperatureChanged{ 450.f });
    REQUIRE(receivedTemp == 450.f);
}

TEST_CASE("EventBus supports multiple subscribers", "[core][eventbus]")
{
    EventBus bus;

    int callCount = 0;
    bus.subscribe<EngineStarted>([&](const EngineStarted&) { ++callCount; });
    bus.subscribe<EngineStarted>([&](const EngineStarted&) { ++callCount; });

    bus.publish(EngineStarted{ 0 });
    REQUIRE(callCount == 2);
}

TEST_CASE("EventBus unsubscribe stops delivery", "[core][eventbus]")
{
    EventBus bus;

    int callCount = 0;
    auto id = bus.subscribe<EngineStarted>([&](const EngineStarted&) { ++callCount; });
    bus.publish(EngineStarted{ 0 });
    REQUIRE(callCount == 1);

    bus.unsubscribe<EngineStarted>(id);
    bus.publish(EngineStarted{ 0 });
    REQUIRE(callCount == 1);  // no second call
}

TEST_CASE("EventBus with no subscribers does not crash", "[core][eventbus]")
{
    EventBus bus;
    REQUIRE_NOTHROW(bus.publish(TemperatureChanged{ 100.f }));
}
