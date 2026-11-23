#pragma once

#include <map>
#include <vector>

#include "canBus.hpp"
#include "canMessage.hpp"
#include "canNode.hpp"
#include "time.hpp"

struct CanBustNodeSample {
    Tick timestamp{};
    double load_percent{0.0};
};

class CanNetwork {
public:
    explicit CanNetwork(const Tick bus_load_sample_period = std::chrono::milliseconds{10})
        : bus_load_sample_period(bus_load_sample_period) {}
    CanNetwork() = default;

    void tick();
    void run(Tick tick_count);
    void reset();
    void add_node(CanNode n);

    [[nodiscard]] CanBus& bus() { return this->m_bus; }
    [[nodiscard]] std::map<std::uint16_t, CanMessageStatistics> collect_statistics() const;
    [[nodiscard]] const std::vector<CanBustNodeSample>& bus_load_statistics() const { return this->bus_load; }
    [[nodiscard]] const auto& get_message_registry() const { return this->message_registry; }

    static constexpr auto BIT_TIME = Tick{2}; /// How long each bit takes to transmit at 500 Kbits / s

private:
    void collect_bus_load_statistics();

    Tick t{};
    std::vector<CanNode> nodes{};
    std::map<std::uint16_t, CanMessageMetadata> message_registry{};
    CanBus m_bus{};

    Tick bus_load_sample_period{std::chrono::microseconds{10'000}};
    std::size_t bus_transmission_bits_in_window{0};
    std::vector<CanBustNodeSample> bus_load{};
};
