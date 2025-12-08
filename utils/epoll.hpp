#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <algorithm>
#include <unordered_map>
#include <sys/epoll.h>
#include <expected>
#include <cstdint>
#include <vector>
#include <functional>
#include <string>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <cerrno>

namespace server {

struct CpuState {
    float usage   { 0.0f };
    float user    { 0.0f };
    float system  { 0.0f };
    float idle    { 0.0f };
};

struct MemState {
    float used_mb      { 0.0f };
    float free_mb      { 0.0f };
    float swap_total   { 0.0f };
    float swap_free    { 0.0f };
};

struct ClientState {
    std::string ip;
    CpuState cpu;
    MemState mem;
};

// Clase para guardar todo lo relacionado a cada cliente que se conecta al servidor.
class Client {
public:
    ClientState& state() { return m_state; }
    std::string& ip() { return m_ip; }

    Client(int fd, std::string_view ip) 
        : m_ip(std::move(ip)) {
        m_fd.reserve(2);
        m_fd.emplace_back(fd);
        m_state.ip = m_ip;
    }

    size_t fdSize(){
        return (m_fd.size());
    }

    void addFd(int fd){
        m_fd.emplace_back(fd);
        return;
    }

    bool isKnownFd(int fd){
        auto it = std::find(m_fd.begin(), m_fd.end(), fd);
        return it != m_fd.end();
    }

    void updateState(std::string type, std::istringstream& ss){

        if (type == "CPU") {
            std::string usage, user, system, idle;
            std::getline(ss, usage, ';');
            std::getline(ss, user, ';');
            std::getline(ss, system, ';');
            std::getline(ss, idle, ';');
            
            m_state.cpu.usage  = std::stof(usage);
            m_state.cpu.user   = std::stof(user);
            m_state.cpu.system = std::stof(system);
            m_state.cpu.idle   = std::stof(idle);
        }

        else if (type == "MEM") {

            std::string used, free_mem, swap_total, swap_free;
            std::getline(ss, used, ';');
            std::getline(ss, free_mem, ';');
            std::getline(ss, swap_total, ';');
            std::getline(ss, swap_free, ';');

            m_state.mem.used_mb    = std::stof(used);
            m_state.mem.free_mb    = std::stof(free_mem);
            m_state.mem.swap_total = std::stof(swap_total);
            m_state.mem.swap_free  = std::stof(swap_free);
        }
    }

private:
    std::vector<int> m_fd;
    std::string m_ip;
    ClientState m_state;
};

} // namespace server

namespace Epoll {

template<typename T>
using result = std::expected<T, int>;

constexpr int MAX_EVENTS = 255;

class Reactor {
public:
    Reactor(int serverFd) {
        m_epollfd = epoll_create1(EPOLL_CLOEXEC);
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = serverFd;
        epoll_ctl(m_epollfd, EPOLL_CTL_ADD, serverFd, &ev);
    }

    ~Reactor() {
        if (m_epollfd >= 0) {
            close(m_epollfd);
        }
    }

    server::Client& addClient(int fd, std::string_view ip) {
        clients.emplace(std::string(ip), server::Client(fd, ip));
        return clients.find(std::string(ip))->second;
    }

    result<server::Client*> getClient(std::string& ip) {
        auto it = clients.find(ip);
        if (it != clients.end()){
            return &it->second;
        }
        return std::unexpected(0);
    }

    void processLine(const std::string& line, int fd) {
        std::istringstream ss(line);
        std::string type, ip;
        
        std::getline(ss, type, ';');
        std::getline(ss, ip, ';');

        auto resultClient = getClient(ip);
        if (!resultClient){
            auto& client = addClient(fd, ip);
            client.updateState(type, ss);
            return;
        }
        auto& client = *resultClient.value();
        if (client.isKnownFd(fd)){
            client.updateState(type, ss);
            return;
        }
        if (client.fdSize() == 2){
            return;
        }
        client.addFd(fd);
        client.updateState(type, ss);
        return;
    }

    void watchFd(int fd){
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev);
    }

    void run(std::function<void(int fd, uint32_t events)> handler) {
        m_running = true;
        std::vector<epoll_event> events(MAX_EVENTS);

        while (m_running) {
            int nfds = epoll_wait(m_epollfd, events.data(), MAX_EVENTS, -1);
            
            if (nfds < 0) {
                if (errno == EINTR) continue;
                break;
            }

            for (int i = 0; i < nfds; ++i) {
                handler(events[i].data.fd, events[i].events);
            }
        }
    }

    void stop() {
        m_running = false;
    }

    auto& getClientsMap(){
        return clients;
    }

private:

    int m_epollfd { -1 };
    bool m_running { false };
    std::unordered_map<std::string, server::Client> clients;    // <ip, client>
    int nextId { 0 };
};

} // namespace Epoll

#endif // EPOLL_HPP