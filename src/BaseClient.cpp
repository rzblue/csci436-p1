#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "BaseClient.hpp"


BaseClient::BaseClient(const std::string& ip, int port)
    : server_ip{ip}, server_port{port} {}

BaseClient::~BaseClient() {
    disconnect();
}

void BaseClient::start() {
    // Create a TCP Socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Error: Failed to create socket\n";
        return;
    }

    // Prepare the sockaddr_in Structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Convert IP Address from Text to Binary Form
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Error: Invalid server IP address\n";
        return;
    }

    // Connect to the Server via the Socket
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Failed to connect to server\n";
        disconnect();
        return;
    }
    std::cout << "Connected to server at " << server_ip << ":" << server_port << "\n";

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