#pragma once
#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <cassert>

namespace a320 {

// Synchronous, single-threaded typed event bus.
// All publish/subscribe calls must happen on the flight loop thread.
class EventBus {
public:
    using HandlerID = uint32_t;

    template<typename Event>
    HandlerID subscribe(std::function<void(const Event&)> handler) {
        auto& vec = m_handlers[typeid(Event)];
        HandlerID id = m_nextId++;
        vec.push_back({ id, [h = std::move(handler)](const void* e) {
            h(*static_cast<const Event*>(e));
        }});
        return id;
    }

    template<typename Event>
    void unsubscribe(HandlerID id) {
        auto it = m_handlers.find(typeid(Event));
        if (it == m_handlers.end()) return;
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [id](const Entry& e) { return e.id == id; }), vec.end());
    }

    template<typename Event>
    void publish(const Event& event) {
        auto it = m_handlers.find(typeid(Event));
        if (it == m_handlers.end()) return;
        for (auto& entry : it->second) entry.fn(&event);
    }

    static EventBus& instance() {
        static EventBus bus;
        return bus;
    }

private:
    struct Entry {
        HandlerID id;
        std::function<void(const void*)> fn;
    };
    std::unordered_map<std::type_index, std::vector<Entry>> m_handlers;
    HandlerID m_nextId = 1;
};

} // namespace a320

#define A320_SUBSCRIBE(EventType, handler) \
    ::a320::EventBus::instance().subscribe<EventType>(handler)
#define A320_PUBLISH(...) \
    ::a320::EventBus::instance().publish(__VA_ARGS__)
