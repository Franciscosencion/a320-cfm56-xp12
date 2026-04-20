#pragma once
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <functional>

// Forward-declare Asio types to keep this header clean
namespace asio { class io_context; }

namespace a320 {

// Incoming hardware/MCDU event from a connected client
struct HardwareEvent {
    uint16_t    type;   // proto::PacketType cast to uint16_t
    std::vector<uint8_t> payload;
};

using HardwareEventCallback = std::function<void(const HardwareEvent&)>;

// NetworkServer runs a dedicated background thread.
// The main flight loop thread calls pushDisplayData() to enqueue outbound packets.
// Inbound events are queued and drained by drainEvents() on the flight loop thread.
class NetworkServer {
public:
    NetworkServer(uint16_t udpPort, uint16_t tcpPort);
    ~NetworkServer();

    void start();
    void stop();

    // Called from flight loop thread — enqueues a UDP broadcast to all clients
    void pushDisplayData(const void* data, size_t bytes);

    // Called from flight loop thread — drains inbound events and fires callbacks
    void drainEvents(const HardwareEventCallback& cb);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace a320
