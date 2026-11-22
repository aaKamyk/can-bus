#include "canFrame.hpp"

namespace {
    struct CanFrameBuilder {
        std::vector<Bit>& packet_bits;
        Bit last_bit{Bit::ONE};
        std::uint8_t streak{0};

        void append_bit(const Bit b) {
            this->packet_bits.emplace_back(b);

            if (this->streak == 0) {
                this->last_bit = b;
                this->streak++;
                return;
            }

            if (this->last_bit == b) {
                this->streak++;
                if (this->streak >= 5) {
                    const Bit stuff_bit = (b == Bit::ZERO) ? Bit::ONE : Bit::ZERO;
                    this->packet_bits.emplace_back(stuff_bit);
                    this->last_bit = stuff_bit;
                    this->streak = 1;
                }
            } else {
                this->last_bit = b;
                this->streak = 1;
            }
        }

        void append_bit_unstuffed(const Bit b) const {
            this->packet_bits.emplace_back(b);
        }

        void append_uint(const std::uint64_t data, std::size_t bits_count) {
            bits_count = std::min(bits_count, std::size_t{64});
            // We want to get masks 2^0, 2^1, ..., 2^n-1
            // But we iterate from n to 1 to avoid unsigned underflow
            for (std::size_t idx = bits_count; idx > 0; idx--) {
                const std::uint64_t mask = 1ull << (idx - 1);
                const Bit bit = (data & mask) == 0 ? Bit::ZERO : Bit::ONE;
                this->append_bit(bit);
            }
        }

        void append_bytes(const std::span<const std::uint8_t> data, const std::size_t bits_count) {
            const std::size_t total_bits = std::min(bits_count, data.size() * 8);
            std::size_t remaining = total_bits;
            for (const std::uint8_t byte : data) {
                if (remaining == 0) { break; }
                const std::size_t bits_in_current_byte = std::min(remaining, std::size_t{8});
                this->append_uint(byte, bits_in_current_byte);
                remaining -= bits_in_current_byte;
            }
        }
    };
}

std::optional<Bit> CanFrame::current_bit() const {
    if (this->sent_bits >= this->bits.size())
    { return std::nullopt; }
    return this->bits[this->sent_bits];
}

std::optional<Bit> CanFrame::advance_bit() {
    if (this->sent_bits >= this->bits.size())
    { return std::nullopt; }
    return this->bits[this->sent_bits++];
}

void CanFrame::reset_progress() {
    this->sent_bits = 0;
}

std::optional<CanFrame> CanFrame::new_packet(
    const std::uint16_t id,
    const std::uint8_t dlc,
    const std::span<const std::uint8_t> message_bits
) {
    static constexpr std::uint16_t MAX_ID = 0x07FF;  // 11 bits set
    if (id > MAX_ID || dlc > MAX_DLC || message_bits.size() < dlc) { return std::nullopt; }

    static constexpr std::uint8_t MAX_FRAME_BITS = 140;
    static constexpr std::uint8_t ID_BITS  = 11;
    static constexpr std::uint8_t DLC_BITS = 4;
    static constexpr std::uint8_t CRC_BITS = 15;
    static constexpr std::uint8_t EOF_BITS = 7;

    static constexpr auto SOF{Bit::ZERO}; // Start of frame
    static constexpr auto RTR{Bit::ZERO}; // Identifies the frame as a data frame
    static constexpr auto IDE{Bit::ZERO}; // Identifies the standard as CAN2.0A
    static constexpr auto r0 {Bit::ZERO}; // Reserved bit
    static constexpr auto DELIMITER_BIT{Bit::ONE};
    static constexpr auto ACK{Bit::ONE};  //
    static constexpr int  CRC{0};

    std::vector<Bit> packet_bits{};
    packet_bits.reserve(MAX_FRAME_BITS);
    CanFrameBuilder builder{ .packet_bits = packet_bits };

    // Start of frame
    builder.append_bit(SOF);
    // Arbitration field
    builder.append_uint(id, ID_BITS);
    builder.append_bit(RTR);
    const std::size_t arb_bits = packet_bits.size();
    // Control field
    builder.append_bit(IDE);
    builder.append_bit(r0);
    builder.append_uint(dlc, DLC_BITS);
    // Data field
    builder.append_bytes(message_bits, dlc * 8);
    // CRC field
    builder.append_uint(CRC, CRC_BITS);
    builder.append_bit_unstuffed(DELIMITER_BIT); // Stop stuffing from here
    // ACK field
    builder.append_bit_unstuffed(ACK);
    builder.append_bit_unstuffed(DELIMITER_BIT);
    // EOF field
    for (std::uint8_t i = 0; i < EOF_BITS; i++) {
        builder.append_bit_unstuffed(Bit::ONE);
    }

    return CanFrame {
        .bits = std::move(packet_bits),
        .sent_bits = 0,
        .arbitration_bits = arb_bits,
    };
}
