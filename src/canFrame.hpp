#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

inline constexpr std::uint8_t MAX_DLC = 8;

enum class Bit : bool {
    ZERO = false,
    ONE  = true,
};

struct CanFrame {
    std::vector<Bit>   bits{};
    std::size_t        sent_bits{0};
    std::size_t        arbitration_bits{0};

    [[nodiscard]]
    std::optional<Bit> current_bit() const;
    std::optional<Bit> advance_bit();
    void reset_progress();

    static std::optional<CanFrame> new_packet(
        std::uint16_t id,
        std::uint8_t dlc,
        std::span<const std::uint8_t> message_bits
    );
};
