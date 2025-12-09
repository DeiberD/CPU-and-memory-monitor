#include "../utils/epoll.hpp"
#include "../utils/sockets.hpp"
#include "../utils/visualize.hpp"
#include <iostream>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <puerto>\n";
        return 1;
    }

    int puerto = std::stoi(argv[1]);
    if (puerto <= 0 || puerto > 65535) {
        std::cerr << "Puerto inválido\n";
        return 1;
    }

    auto serverResult = server::ServerTCP::bindServer(
        server::Endpoint({0, 0, 0, 0}, puerto)
    );

    if (!serverResult) {
        std::cerr << "Error al crear el servidor\n";
        return 1;
    }

    auto& tcpServer = serverResult.value();
    int serverFd = tcpServer.fd();

    std::cout << "Servidor iniciado en puerto " << puerto << "\n";

    Epoll::Reactor reactor(serverFd);

    reactor.run([&](int fd, uint32_t events) {
        if (fd == serverFd) {
            int newFd = tcpServer.accept();
            reactor.watchFd(newFd);
        } 
        else if (events & EPOLLIN) {
            std::array<char, 4096> buffer{};
            ssize_t bytes = read(fd, buffer.data(), buffer.size() - 1);
            if (bytes <= 0) {
            epoll_ctl(reactor.m_epollfd, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            return;
            }
            std::string line(buffer.data(), bytes);
            reactor.processLine(line, fd);
            visualize::displayClients(reactor.getClientsMap());
        }
    });

    return 0;
}
