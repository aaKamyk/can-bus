#include <format>
#include <fstream>
#include <iostream>

#include "canNetwork.hpp"

using namespace std::chrono_literals;

int main() {
    CanNetwork network{10ms};

    CanNode engine{"silnik", network.bus()};
    engine.add_message("temperatura", { .id = 0x010, .data_bytes = 4, .period = 10ms, .deadline = 50ms});
    engine.add_message("ciśnienie", { .id = 0x020, .data_bytes = 4, .period = 20ms, .deadline = 10ms });

    CanNode power{"zasilanie", network.bus()};
    power.add_message("napięcie akumulatora", { .id = 0x100, .data_bytes = 2, .period = 100ms });

    CanNode nav{"nawigacja", network.bus()};
    nav.add_message("pozycja GPS", { .id = 0x200, .data_bytes = 8, .period = 200ms, .deadline = 100ms });

    CanNode diag{"diagnostyka", network.bus()};
    diag.add_message("status systemu", { .id = 0x300, .data_bytes = 4, .period = 500ms });

    CanNode on_board_computer{"komputer pokładowy", network.bus()};

    network.add_node(std::move(engine));
    network.add_node(std::move(power));
    network.add_node(std::move(nav));
    network.add_node(std::move(diag));
    network.add_node(std::move(on_board_computer));

    network.run(10s);

    for (auto stats = network.collect_statistics();
        const auto& [id, stat] : stats) {
        std::cout << std::format(
            "Id: {:#06x}\n"
            "Delivered count: {}\n"
            "Missed deadlines: {}\n"
            "Average latency: {}\n"
            "Max latency: {}\n",
            id, stat.delivered, stat.missed_deadlines, stat.average_latency(), stat.max_latency
        );
    }


    std::ofstream bus_load_statistics_file{"bus_load.csv"};
    bus_load_statistics_file << "time_us;bus_load_percent\n";
    for (const auto [timestamp, load_percent] : network.bus_load_statistics()) {
        bus_load_statistics_file <<
            std::format("{};{:.2f}\n", timestamp.count(), load_percent);
    }
}