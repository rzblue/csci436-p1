#include "HTTPSTunnel.hpp"
#include "NetworkUtils.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <iostream>
#include <algorithm>

// ====================================================================================================
// Public Methods
// ====================================================================================================

bool HTTPSTunnel::establish(int client_fd, const std::string& host, int port) {
    return establish(client_fd, host, port, "HTTP/1.1 200 Connection Established\r\n\r\n");
}

bool HTTPSTunnel::establish(int client_fd, 
                            const std::string& host, 
                            int port,
                            const std::string& success_response) {
    std::cout << "[HTTPSTunnel] Establishing tunnel to " << host << ":" << port << "\n";

    // Step 1: Connect to destination server using NetworkUtils
    int server_fd = NetworkUtils::connectToHost(host, port);
    if (server_fd < 0) {
        std::cerr << "[HTTPSTunnel] Failed to connect to " << host << ":" << port << "\n";
        return false;
    }

    std::cout << "[HTTPSTunnel] Connected to " << host << ":" << port << "\n";

    // Step 2: Inform client that tunnel is ready using NetworkUtils
    if (!NetworkUtils::sendData(client_fd, success_response)) {
        std::cerr << "[HTTPSTunnel] Failed to send success response to client\n";
        close(server_fd);
        return false;
    }

    std::cout << "[HTTPSTunnel] Tunnel established, forwarding traffic...\n";

    // Step 3: Forward data bidirectionally
    forwardTraffic(client_fd, server_fd);

    // Step 4: Cleanup
    close(server_fd);
    std::cout << "[HTTPSTunnel] Tunnel closed\n";

    return true;
}

// ====================================================================================================
// Private Helper Methods
// ====================================================================================================

void HTTPSTunnel::forwardTraffic(int client_fd, int server_fd) {
    fd_set read_fds;
    char buf[8192];
    int max_fd = std::max(client_fd, server_fd) + 1;

    while (true) {
        // Setup file descriptor set for select()
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        FD_SET(server_fd, &read_fds);

        // Wait for activity on either socket
        int activity = select(max_fd, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            std::cerr << "[HTTPSTunnel] Select error: " << NetworkUtils::getLastError() << "\n";
            break;
        }

        // Check if client has data to send to server
        if (FD_ISSET(client_fd, &read_fds)) {
            ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
            
            if (n <= 0) {
                if (n < 0) {
                    std::cerr << "[HTTPSTunnel] Client recv error: " 
                              << NetworkUtils::getLastError() << "\n";
                } else {
                    std::cout << "[HTTPSTunnel] Client closed connection\n";
                }
                break;
            }
            
            // Use NetworkUtils to send data to server
            if (!NetworkUtils::sendData(server_fd, buf, n)) {
                std::cerr << "[HTTPSTunnel] Failed to forward client data to server\n";
                break;
            }
        }

        // Check if server has data to send to client
        if (FD_ISSET(server_fd, &read_fds)) {
            ssize_t n = recv(server_fd, buf, sizeof(buf), 0);
            
            if (n <= 0) {
                if (n < 0) {
                    std::cerr << "[HTTPSTunnel] Server recv error: " 
                              << NetworkUtils::getLastError() << "\n";
                } else {
                    std::cout << "[HTTPSTunnel] Server closed connection\n";
                }
                break;
            }
            
            // Use NetworkUtils to send data to client
            if (!NetworkUtils::sendData(client_fd, buf, n)) {
                std::cerr << "[HTTPSTunnel] Failed to forward server data to client\n";
                break;
            }
        }
    }
}