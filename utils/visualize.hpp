#ifndef VISUALIZE_HPP
#define VISUALIZE_HPP

#include "epoll.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>
#include <iomanip>

namespace visualize {

void displayClients(std::unordered_map<std::string, server::Client>& clients) {
    std::cout << "\033[2J\033[H";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Clientes: " << clients.size() << "\n\n";

    if (clients.empty()) {
        std::cout << "Esperando conexiones...\n";
    } else {
        std::cout << std::left << std::setw(18) << "IP"
                  << std::right << std::setw(8) << "CPU%"
                  << std::setw(8) << "User%"
                  << std::setw(8) << "Sys%"
                  << std::setw(8) << "Idle%"
                  << std::setw(10) << "MemUsed"
                  << std::setw(10) << "MemFree"
                  << std::setw(10) << "SwapTot"
                  << std::setw(10) << "SwapFree"
                  << "\n";
        
        std::cout << std::string(80, '-') << "\n";

        for (auto& [ip, client] : clients) {
            auto& state = client.state();
            std::cout << std::left << std::setw(18) << state.ip
                      << std::right << std::setw(8) << state.cpu.usage
                      << std::setw(8) << state.cpu.user
                      << std::setw(8) << state.cpu.system
                      << std::setw(8) << state.cpu.idle
                      << std::setw(10) << state.mem.used_mb
                      << std::setw(10) << state.mem.free_mb
                      << std::setw(10) << state.mem.swap_total
                      << std::setw(10) << state.mem.swap_free
                      << "\n";
        }
    }

    std::cout.flush();
}

} // namespace visualize

#endif // VISUALIZE_HPP
