#pragma once

#include <cstdint>

enum class CanWireState : std::uint8_t {
    LOW,    // 1.5V - driven low
    IDLE,   // 2.5V
    HIGH,   // 3.5V - driven high
};

struct CanBus {
    CanWireState can_l = CanWireState::IDLE;
    CanWireState can_h = CanWireState::IDLE;

    constexpr void release() {
        this->can_l = CanWireState::IDLE;
        this->can_h = CanWireState::IDLE;
    }

    constexpr void drive_dominant() {
        this->can_l = CanWireState::LOW;
        this->can_h = CanWireState::HIGH;
    }

    [[nodiscard]] constexpr bool is_dominant() const {
        return this->can_h == CanWireState::HIGH &&
               this->can_l == CanWireState::LOW;
    }
};
