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
    visualize::displayClients(reactor.getClients());

    reactor.run([&](int fd, uint32_t events) {
        if (fd == serverFd) {
            auto acceptResult = tcpServer.accept();
            if (acceptResult) {
                auto& connection = acceptResult.value();
                reactor.addClient(connection.fd, connection.ip, EPOLLIN);
                visualize::displayClients(reactor.getClients());
            }
        } 
        else if (events & EPOLLIN) {
            auto& client = reactor.getClient(fd);

            std::array<char, 4096> buffer{};
            ssize_t bytes = read(fd, buffer.data(), buffer.size() - 1);

            if (bytes <= 0) {
                reactor.removeClient(fd);
                close(fd);
                visualize::displayClients(reactor.getClients());
            } else {
                std::string line(buffer.data(), bytes);
                if (!line.empty() && line.back() == '\n') line.pop_back();
                if (!line.empty() && line.back() == '\r') line.pop_back();
                client.updateState(line);
                visualize::displayClients(reactor.getClients());
            }
        }
    });

    return 0;
}
