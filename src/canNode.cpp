#include "canNode.hpp"

#include <array>
#include <ranges>

#include "canFrame.hpp"

using namespace std::chrono_literals;

void CanNode::tick(const Tick current_tick) {
    for (const auto& [id, data_bytes, period, deadline] : this->messages | std::views::values) {
        if (current_tick % period == 0us) {
            if (auto packet = CanFrame::new_packet(id, data_bytes, std::array<std::uint8_t, 8>{})) {
                this->tx_queue.emplace(PendingCanFrame{
                    .frame = std::move(packet.value()),
                    .id = id,
                    .created_at = current_tick,
                    .due_at = deadline.has_value() ?
                        std::optional{current_tick + deadline.value()} :
                        std::nullopt,
                });
            }
        }
    }

    // Set active packet if not doing anything and one is waiting in the queue
    if (this->state == CanNodeState::Idle && !this->tx_queue.empty()) {
        // Ugly const_cast because priority queue doesnt have a non const& overload
        this->current_frame = std::move(const_cast<PendingCanFrame&>(this->tx_queue.top()));
        this->tx_queue.pop();
        this->state = CanNodeState::WaitingForBus;
    }

    // If we detect that the bus is idle attempt to transmit
    if (this->state == CanNodeState::WaitingForBus && this->recessive_bits >= 11) {
        this->state = CanNodeState::Transmitting;
    }
}

void CanNode::drive() const {
    if (this->state != CanNodeState::Transmitting ||
       !this->current_frame.has_value()) { return; }

    if (const auto bit = this->current_frame->frame.current_bit();
        bit.has_value() && bit.value() == Bit::ZERO) {
        this->bus.drive_dominant();
    }
}

void CanNode::sample(const Tick current_tick) {
    const bool bus_dominant = this->bus.is_dominant();

    if (bus_dominant) {
        this->recessive_bits = 0;
    } else if (this->recessive_bits <= BUS_IDLE_BITS) {
        this->recessive_bits++;
    }

    if (this->state != CanNodeState::Transmitting ||
       !this->current_frame.has_value()) { return; }

    const auto sent_bit = this->current_frame->frame.current_bit();
    if (!sent_bit.has_value()) { return; }

    // Arbitration (in range [SOF; RTF])
    if (this->current_frame->frame.sent_bits < this->current_frame->frame.arbitration_bits &&
        sent_bit.value() == Bit::ONE && bus_dominant) {
        this->current_frame->frame.reset_progress();
        this->state = CanNodeState::WaitingForBus;
        return;
    }

    // Advance the bit after arbitration check succeeded
    this->current_frame->frame.advance_bit();
    if (!this->current_frame->frame.current_bit().has_value()) {
        this->diagnostics.emplace_back(CanMessageDiagnostic{
            .id = this->current_frame->id,
            .created_at = this->current_frame->created_at,
            .delivered_at = current_tick,
        });

        this->current_frame = std::nullopt;
        this->state = CanNodeState::Idle;
    }
}

void CanNode::reset() {
    this->state = CanNodeState::Idle;
    this->recessive_bits = BUS_IDLE_BITS;
    this->current_frame = std::nullopt;
    this->diagnostics.clear();
}

void CanNode::add_message(CanMessageName msg_name, CanMessageConfig config) {
    this->messages.emplace(std::move(msg_name), config);
}
