#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "ProxyServer.hpp"
#include "Protocol.hpp"


void ProxyServer::handleRequest(int client_fd) {
    // Read Proxy Header
    Protocol::ProxyHeader header{};
    ssize_t bytes_read = recv(client_fd, &header, sizeof(header), MSG_WAITALL);
    if (bytes_read != sizeof(header)) {
        std::cerr << "[ProxyServer] Invalid or incomplete proxy header\n";
        return;
    }

    // Extract Destination Info
    std::string dest_ip(inet_ntoa(header.dest_addr));
    int dest_port = ntohs(header.dest_port);
    std::cout << "[ProxyServer] Connecting to destination "
              << dest_ip << ":" << dest_port << "\n";

    // Create Socket to Destination
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[ProxyServer] Failed to create server socket\n";
        return;
    }

    // Set Up Destination Address Structure
    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    dest_addr.sin_addr = header.dest_addr;

    // Connect to Destination Server
    if (connect(server_fd, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        std::cerr << "[ProxyServer] Failed to connect to "
                  << dest_ip << ":" << dest_port << "\n";
        close(server_fd);
        return;
    }
    std::cout << "[ProxyServer] Connected to destination.\n";

    // Relay loop
    fd_set fds;
    char relay_buf[4096];
    while (true) {
        // Monitor Both Sockets
        FD_ZERO(&fds);
        FD_SET(client_fd, &fds);
        FD_SET(server_fd, &fds);
        int max_fd = std::max(client_fd, server_fd) + 1;

        // Break If No Activity on Either Socket
        if (select(max_fd, &fds, nullptr, nullptr, nullptr) < 0) break;

        // Client → Server
        if (FD_ISSET(client_fd, &fds)) {
            ssize_t n = recv(client_fd, relay_buf, sizeof(relay_buf), 0);
            std::cout << "[ProxyServer] Relayed " << n << " bytes from client to server.\n";
            if (n <= 0) break;
            send(server_fd, relay_buf, n, 0);
        }

        // Server → Client
        if (FD_ISSET(server_fd, &fds)) {
            ssize_t n = recv(server_fd, relay_buf, sizeof(relay_buf), 0);
            std::cout << "[ProxyServer] Relayed " << n << " bytes from server to client.\n";
            if (n <= 0) break;
            send(client_fd, relay_buf, n, 0);
        }
    }

    close(server_fd);
    std::cout << "[ProxyServer] Connection closed.\n";
}
