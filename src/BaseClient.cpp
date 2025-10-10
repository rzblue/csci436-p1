#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "BaseClient.hpp"
#include "Protocol.hpp"


BaseClient::BaseClient(const std::string& ip, int port)
    : server_ip{ip}, server_port{port} {}


BaseClient::BaseClient(const std::string& dest_ip, int dest_port,
                       const std::string& proxy_ip, int proxy_port)
    : server_ip{dest_ip}, server_port{dest_port},
      use_proxy{true}, proxy_ip{proxy_ip}, proxy_port{proxy_port} {}


BaseClient::~BaseClient() {
    disconnect();
}


void BaseClient::start() {
    // Determine Connection Parameters (Proxy or Direct)
    std::string connect_ip = use_proxy ? proxy_ip : server_ip;
    int connect_port = use_proxy ? proxy_port : server_port;

    // Create a TCP Socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Error: Failed to create socket\n";
        return;
    }

    // Prepare the sockaddr_in Structure
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(connect_port);

    // Convert IP Address from Text to Binary Form
    if (inet_pton(AF_INET, connect_ip.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Error: Invalid server IP address\n";
        disconnect();
        return;
    }

    // Connect to the Server via the Socket
    if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error: Failed to connect to " 
                  << (use_proxy ? "proxy" : "server") << " at "
                  << connect_ip << ":" << connect_port << "\n";
        disconnect();
        return;
    }
    std::cout << "Connected to " 
              << (use_proxy ? "proxy" : "server") 
              << " at " << connect_ip << ":" << connect_port << "\n";

    if (use_proxy) {
        // Prepare Destination Address
        sockaddr_in dest_addr{};
        inet_pton(AF_INET, server_ip.c_str(), &dest_addr.sin_addr);

        // Prepare Proxy Header
        Protocol::ProxyHeader header;
        header.dest_addr = dest_addr.sin_addr;
        header.dest_port = htons(server_port);

        // Send Proxy Header
        ssize_t sent = send(socket_fd, &header, sizeof(header), 0);
        if (sent != sizeof(header)) {
            std::cerr << "Error: Failed to send proxy header\n";
            disconnect();
            return;
        }
        std::cout << "Binary proxy header sent ("
                  << server_ip << ":" << server_port << ")\n";
    }

    // Make the Request (Implemented by Derived Class)
    makeRequest();
}


void BaseClient::disconnect() {
    if (socket_fd != -1) {
        close(socket_fd);
        socket_fd = -1;
        std::cout << "Disconnected from server\n";
    }
}