#include "NetworkUtils.hpp"

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

// ====================================================================================================
// Connection Management
// ====================================================================================================

int NetworkUtils::connectToHost(const std::string& host, int port) {
    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP

    struct addrinfo* res = nullptr;
    std::string port_str = std::to_string(port);

    // Perform DNS resolution
    int err = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (err != 0) {
        std::cerr << "[NetworkUtils] DNS resolution failed for " << host
                  << ": " << gai_strerror(err) << "\n";
        return -1;
    }

    // Create socket
    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd < 0) {
        std::cerr << "[NetworkUtils] Failed to create socket: " 
                  << strerror(errno) << "\n";
        freeaddrinfo(res);
        return -1;
    }

    // Connect to remote host
    if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "[NetworkUtils] Failed to connect to " 
                  << host << ":" << port 
                  << " (" << strerror(errno) << ")\n";
        close(sock_fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock_fd;
}

// ====================================================================================================
// Data Transmission
// ====================================================================================================

bool NetworkUtils::sendData(int fd, const char* data, size_t length) {
    size_t total_sent = 0;

    while (total_sent < length) {
        ssize_t sent = send(fd, data + total_sent, length - total_sent, 0);
        
        if (sent < 0) {
            std::cerr << "[NetworkUtils] Send failed: " << strerror(errno) << "\n";
            return false;
        }
        
        if (sent == 0) {
            std::cerr << "[NetworkUtils] Connection closed during send\n";
            return false;
        }
        
        total_sent += sent;
    }
    
    return true;
}

bool NetworkUtils::sendData(int fd, const std::string& data) {
    return sendData(fd, data.c_str(), data.size());
}

// ====================================================================================================
// Data Reception
// ====================================================================================================

ssize_t NetworkUtils::receiveData(int fd, char* buffer, size_t max_length) {
    ssize_t received = recv(fd, buffer, max_length, 0);
    
    if (received < 0) {
        std::cerr << "[NetworkUtils] Receive failed: " << strerror(errno) << "\n";
        return -1;
    }
    
    if (received == 0) {
        // Connection closed by peer (not necessarily an error)
        return 0;
    }
    
    return received;
}

// ====================================================================================================
// Socket Configuration
// ====================================================================================================

bool NetworkUtils::setSocketTimeout(int fd, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    // Set receive timeout
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "[NetworkUtils] Failed to set receive timeout: " 
                  << strerror(errno) << "\n";
        return false;
    }

    // Set send timeout
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "[NetworkUtils] Failed to set send timeout: " 
                  << strerror(errno) << "\n";
        return false;
    }

    return true;
}

// ====================================================================================================
// Error Handling
// ====================================================================================================

std::string NetworkUtils::getLastError() {
    return std::string(strerror(errno));
}