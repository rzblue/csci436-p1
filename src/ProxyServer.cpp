#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "ProxyServer.hpp"
#include "Protocol.hpp"

#include "Logger.hpp"

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

    // Relay Loop
    fd_set fds;  // Declare File Descriptor Set
    char relay_buffer[4096];
    while (true) {
        FD_ZERO(&fds);                                      // Clear the File Descriptor Set
        FD_SET(client_fd, &fds);                            // Add Client Socket to Set
        FD_SET(server_fd, &fds);                            // Add Server Socket to Set
        int max_fd = std::max(client_fd, server_fd) + 1;    // Compute Max FD for select()

        // Wait for Activity on Either Socket
        // Block Until Activity is Detected
        // Break Loop on Error
        if (select(max_fd, &fds, nullptr, nullptr, nullptr) < 0) break;

        // Client → Server
        // Check if Client Socket is Ready for Reading
        if (FD_ISSET(client_fd, &fds)) {
            // Read Data from Client and Forward to Server
            ssize_t n = recv(client_fd, relay_buffer, sizeof(relay_buffer), 0);
            std::cout << "[ProxyServer] Relayed " << n << " bytes from client to server.\n";
            if (n <= 0) break;
            send(server_fd, relay_buffer, n, 0);
        }

        // Server → Client
        // Check if Server Socket is Ready for Reading
        if (FD_ISSET(server_fd, &fds)) {
            // Read Data from Server and Forward to Client
            ssize_t n = recv(server_fd, relay_buffer, sizeof(relay_buffer), 0);
            std::cout << "[ProxyServer] Relayed " << n << " bytes from server to client.\n";
            if (n <= 0) break;
            send(client_fd, relay_buffer, n, 0);
        }
    }

    close(server_fd);
    std::cout << "[ProxyServer] Connection closed.\n";
}
