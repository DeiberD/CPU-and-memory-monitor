#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <expected>
#include <array>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

namespace server {

enum class Error {
    UnknownError,
};

template<typename T>
using result = std::expected<T, Error>;

class Endpoint {
public:
    Endpoint(std::array<uint8_t, 4> ip, int port) : m_ip(ip), m_port(port) {}
    
    sockaddr_in toSockaddr() {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(m_port);
        std::memcpy(&addr.sin_addr.s_addr, m_ip.data(), 4);
        return addr;
    }

private:
    std::array<uint8_t, 4> m_ip;
    int m_port;
};

class ServerTCP {
public:
    ServerTCP(int fd) : m_fd(fd) {}

    int fd() const { return m_fd; }

    static result<ServerTCP> bindServer(Endpoint&& endpoint) {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            std::cerr << "[ERROR] no se creó el socket\n";
            return std::unexpected(Error::UnknownError);
        }
        
        int optval = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        
        auto addr = endpoint.toSockaddr();
        if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "[ERROR] No se pudo hacer bind al endpoint deseado\n";
            return std::unexpected(Error::UnknownError);
        }
        if (::listen(fd, 2000) < 0) {
            std::cerr << "[ERROR] error haciendo listen\n";
            return std::unexpected(Error::UnknownError);
        }
        return ServerTCP{fd};
    }

    int accept() {
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        int clientFd = ::accept(m_fd, reinterpret_cast<sockaddr*>(&addr), &len);
        if (clientFd < 0) {
            return clientFd;
        }
        return clientFd;
    }

private:
    int m_fd;
};

} // namespace server

#endif // SOCKETS_HPP
