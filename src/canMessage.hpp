#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "time.hpp"

struct CanMessageConfig {
    std::uint16_t           id{};
    std::uint8_t            data_bytes{};
    Tick                    period{};
    std::optional<Tick>     deadline{std::nullopt};
};

struct CanMessageMetadata {
    std::string             module_name{};
    std::string             message_name{};
    CanMessageConfig        config{};
};

struct CanMessageDiagnostic {
    std::uint16_t           id{};
    Tick                    created_at{};
    Tick                    delivered_at{};

    [[nodiscard]] constexpr Tick latency() const;
};

struct CanMessageStatistics {
    std::uint16_t           id{};
    std::size_t             delivered{0};
    std::size_t             missed_deadlines{0};
    Tick                    total_latency{0};
    Tick                    max_latency{0};

    [[nodiscard]] constexpr double average_latency() const;
};

constexpr double CanMessageStatistics::average_latency() const {
    return delivered > 0 ?
        static_cast<double>(total_latency.count()) / static_cast<double>(delivered) :
        0.0;
}

constexpr Tick CanMessageDiagnostic::latency() const {
    return this->delivered_at - this->created_at;
}
