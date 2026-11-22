#include "canNetwork.hpp"

#include <algorithm>
#include <stdexcept>

using namespace std::chrono_literals;

void CanNetwork::tick() {
    this->m_bus.release();

    for (auto& node : this->nodes) { node.tick(this->t); }
    for (auto& node : this->nodes) { node.drive(); }
    for (auto& node : this->nodes) { node.sample(this->t); }

    this->collect_bus_load_statistics();

    this->t += BIT_TIME;
}

void CanNetwork::run(const Tick tick_count) {
    this->reset();

    while (this->t <= tick_count) {
        this->tick();
    }
}

void CanNetwork::reset() {
    for (auto& node : this->nodes) { node.reset(); }
    this->m_bus.release();
    this->t = 0us;

    this->bus_transmission_bits_in_window = 0;
    this->bus_load.clear();
}

void CanNetwork::add_node(CanNode n) {
    for (const auto& [msg_name, cfg] : n.get_messages()) {
        if (this->message_registry.contains(cfg.id)) {
            throw std::invalid_argument("Duplicate message ID registered on the same network: " + std::to_string(cfg.id));
        }
        this->message_registry.insert({
            cfg.id, CanMessageMetadata{
                .module_name = n.get_name(),
                .message_name = msg_name,
                .config = cfg,
            }});
    }
    this->nodes.emplace_back(std::move(n));
}

std::map<std::uint16_t, CanMessageStatistics> CanNetwork::collect_statistics() const {
    std::map<std::uint16_t, CanMessageStatistics> statistics;

    for (const auto& node : this->nodes) {
        for (const auto& diagnostic : node.get_diagnostics()) {
            // Use square brackets on purpose to create a new entry if encountering a new id
            auto& entry = statistics[diagnostic.id];

            const auto latency = diagnostic.latency();
            const auto deadline = this->message_registry.at(diagnostic.id).config.deadline;

            entry.id = diagnostic.id;
            entry.delivered++;
            entry.total_latency += latency;
            entry.max_latency = std::max(entry.max_latency, latency);
            if (deadline.has_value() && latency > deadline.value()) {
                entry.missed_deadlines++;
            }
        }
    }

    return statistics;
}

void CanNetwork::collect_bus_load_statistics() {
    const bool is_any_transmitting = std::ranges::any_of(
        this->nodes,
        [](const CanNode& node) -> bool { return node.get_state() == CanNodeState::Transmitting; }
    );

    if (is_any_transmitting) {
        this->bus_transmission_bits_in_window++;
    }

    if (this->t > 0us && this->t % this->bus_load_sample_period == 0us) {
        const auto total_bits_in_window = this->bus_load_sample_period / BIT_TIME;
        const double load =
            static_cast<double>(this->bus_transmission_bits_in_window) /
            static_cast<double>(total_bits_in_window) * 100;
        this->bus_load.emplace_back(CanBustNodeSample{
            .timestamp = this->t,
            .load_percent = load,
        });
        this->bus_transmission_bits_in_window = 0;
    }
}
