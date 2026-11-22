#pragma once

#include <functional>
#include <map>
#include <optional>
#include <queue>
#include <string>

#include "canBus.hpp"
#include "canFrame.hpp"
#include "canMessage.hpp"
#include "time.hpp"

enum class QueueingStrategy {
    FIFO,
    IdPriority,
    DeadlinePriority,
};

enum class CanNodeState {
    Idle,
    Transmitting,
    WaitingForBus,
};

class CanNode {
public:
    using CanMessageName = std::string;

    explicit CanNode(CanBus& bus) : bus(bus) {}
             CanNode(std::string name, CanBus& bus) : name(std::move(name)), bus(bus) {}

    void tick(Tick current_tick);
    void drive() const;
    void sample(Tick current_tick);
    void reset();
    void add_message(CanMessageName msg_name, CanMessageConfig config);

    [[nodiscard]] CanNodeState get_state() const { return this->state; }
    [[nodiscard]] const auto&  get_name() const { return this->name; }
    [[nodiscard]] const auto&  get_messages() const { return this->messages; }
    [[nodiscard]] const auto&  get_diagnostics() const { return this->diagnostics; }

private:
    static constexpr std::int32_t BUS_IDLE_BITS = 11;
    struct PendingCanFrame {
        CanFrame            frame{};
        std::uint16_t       id{};
        Tick                created_at{};
        std::optional<Tick> due_at{};
    };

    template <QueueingStrategy strategy>
    struct PendingCanFrameCompare {
        static constexpr bool operator()(const PendingCanFrame& l, const PendingCanFrame& r) {
            if constexpr (strategy == QueueingStrategy::IdPriority)
                /// If equal id's sort by creation time
                { return l.id != r.id ? l.id > r.id : l.created_at > r.created_at; }
            if constexpr (strategy == QueueingStrategy::DeadlinePriority) {
                /// Frame with deadline always goes first
                if (l.due_at.has_value() != r.due_at.has_value()) {
                    return !l.due_at.has_value();
                }
                if (l.due_at.has_value() && r.due_at.has_value() &&
                    l.due_at.value() != r.due_at.value()) {
                    return *l.due_at > *r.due_at;
                }
                /// Break ties by id and creation time
                return l.id != r.id ? l.id > r.id : l.created_at > r.created_at;
            }
            if constexpr (strategy == QueueingStrategy::FIFO)
                { return l.created_at > r.created_at; }

            return false;
        }
    };

    std::string name{};
    std::map<CanMessageName, CanMessageConfig> messages{};
    std::vector<CanMessageDiagnostic> diagnostics{};
    CanBus& bus;
    CanNodeState state{CanNodeState::Idle};

    std::int32_t recessive_bits{BUS_IDLE_BITS};
    std::priority_queue<
        PendingCanFrame,
        std::vector<PendingCanFrame>,
        PendingCanFrameCompare<QueueingStrategy::IdPriority>
    > tx_queue{};
    std::optional<PendingCanFrame> current_frame;
};
