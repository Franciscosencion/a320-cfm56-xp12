#include "NetworkServer.h"
#include "core/Logger.h"
#include "network/Protocol.h"

#include <asio.hpp>
#include <atomic>
#include <deque>

using asio::ip::udp;
using asio::ip::tcp;

namespace a320 {

// ── Ring buffer for display data (flight loop → network thread) ─────────────
static constexpr size_t RING_CAPACITY = 256;

struct RingBuffer {
    struct Slot { std::vector<uint8_t> data; };
    std::array<Slot, RING_CAPACITY> slots;
    std::atomic<uint32_t> writeIdx{0};
    std::atomic<uint32_t> readIdx{0};

    bool push(const void* data, size_t bytes) {
        uint32_t w = writeIdx.load(std::memory_order_relaxed);
        uint32_t next = (w + 1) % RING_CAPACITY;
        if (next == readIdx.load(std::memory_order_acquire)) return false; // full
        slots[w].data.assign(static_cast<const uint8_t*>(data),
                              static_cast<const uint8_t*>(data) + bytes);
        writeIdx.store(next, std::memory_order_release);
        return true;
    }

    bool pop(std::vector<uint8_t>& out) {
        uint32_t r = readIdx.load(std::memory_order_relaxed);
        if (r == writeIdx.load(std::memory_order_acquire)) return false; // empty
        out = std::move(slots[r].data);
        readIdx.store((r + 1) % RING_CAPACITY, std::memory_order_release);
        return true;
    }
};

// ── Impl ────────────────────────────────────────────────────────────────────
struct NetworkServer::Impl {
    uint16_t udpPort;
    uint16_t tcpPort;

    asio::io_context        ioc;
    udp::socket             udpSocket{ioc};
    tcp::acceptor           tcpAcceptor{ioc};
    std::vector<std::shared_ptr<tcp::socket>> tcpClients;

    std::thread             ioThread;
    std::atomic<bool>       running{false};

    RingBuffer              outbound;

    std::mutex              inboundMutex;
    std::deque<HardwareEvent> inboundQueue;

    void sendHeartbeat();
    void acceptLoop();
    void drainOutbound();
};

// ── Public API ───────────────────────────────────────────────────────────────
NetworkServer::NetworkServer(uint16_t udpPort, uint16_t tcpPort)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->udpPort = udpPort;
    m_impl->tcpPort = tcpPort;
}

NetworkServer::~NetworkServer() { stop(); }

void NetworkServer::start()
{
    auto& impl = *m_impl;
    impl.running = true;

    // UDP broadcast socket
    impl.udpSocket.open(udp::v4());
    impl.udpSocket.set_option(asio::socket_base::broadcast(true));
    impl.udpSocket.bind(udp::endpoint(udp::v4(), 0));

    // TCP acceptor
    tcp::endpoint tcpEp(tcp::v4(), impl.tcpPort);
    impl.tcpAcceptor.open(tcpEp.protocol());
    impl.tcpAcceptor.set_option(asio::socket_base::reuse_address(true));
    impl.tcpAcceptor.bind(tcpEp);
    impl.tcpAcceptor.listen();

    impl.acceptLoop();

    impl.ioThread = std::thread([&impl]() {
        LOG_INFO("Network IO thread started (UDP:{} TCP:{})", impl.udpPort, impl.tcpPort);
        impl.ioc.run();
        LOG_INFO("Network IO thread stopped");
    });
}

void NetworkServer::stop()
{
    if (!m_impl->running.exchange(false)) return;
    m_impl->ioc.stop();
    if (m_impl->ioThread.joinable()) m_impl->ioThread.join();
}

void NetworkServer::pushDisplayData(const void* data, size_t bytes)
{
    m_impl->outbound.push(data, bytes);
    // Wake the IO thread to drain — post a handler
    asio::post(m_impl->ioc, [this]() {
        m_impl->drainOutbound();
    });
}

void NetworkServer::drainEvents(const HardwareEventCallback& cb)
{
    std::deque<HardwareEvent> local;
    {
        std::lock_guard lock(m_impl->inboundMutex);
        std::swap(local, m_impl->inboundQueue);
    }
    for (auto& ev : local) cb(ev);
}

// ── Impl internals ───────────────────────────────────────────────────────────
void NetworkServer::Impl::drainOutbound()
{
    std::vector<uint8_t> pkt;
    while (outbound.pop(pkt)) {
        udp::endpoint broadcast(asio::ip::address_v4::broadcast(), udpPort);
        udpSocket.send_to(asio::buffer(pkt), broadcast);
    }
}

void NetworkServer::Impl::acceptLoop()
{
    tcpAcceptor.async_accept([this](asio::error_code ec, tcp::socket sock) {
        if (!ec) {
            LOG_INFO("TCP client connected: {}", sock.remote_endpoint().address().to_string());
            auto sp = std::make_shared<tcp::socket>(std::move(sock));
            tcpClients.push_back(sp);
        }
        if (running) acceptLoop();
    });
}

} // namespace a320
