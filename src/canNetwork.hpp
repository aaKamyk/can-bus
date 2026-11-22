#pragma once

#include <map>
#include <vector>

#include "canBus.hpp"
#include "canMessage.hpp"
#include "canNode.hpp"
#include "time.hpp"

class CanNetwork {
public:
    CanNetwork() = default;

    void tick();
    void run(Tick tick_count);
    void reset();
    void add_node(CanNode n);

    [[nodiscard]] CanBus& bus() { return this->m_bus; }
    [[nodiscard]] std::map<std::uint16_t, CanMessageStatistics> collect_statistics() const;

    static constexpr auto BIT_TIME = Tick{2}; /// How long each bit takes to transmit at 500 Kbits / s

private:
    Tick t{};
    std::vector<CanNode> nodes{};
    std::map<std::uint16_t, CanMessageMetadata> message_registry{};
    CanBus m_bus{};
};
