#include <iostream>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Server.hpp"

Server::Server(int port)
: server_fd{-1},
  port{port} {
    // Create a TCP Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server_fd < 0) {
        std::cerr << "Error: Failed to create socket\n";
        exit(EXIT_FAILURE);
    }

    // Set Socket Options to Allow Reuse of Address
    int opt = 1;
    if (setsockopt(this->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: Failed to set socket options\n";
        close(this->server_fd);
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in Structure
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Listen on all interfaces
    server_addr.sin_port = htons(port);         // Port (network byte order)

    // Bind the Socket to the Port
    if (bind(this->server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Failed to bind socket to port\n";
        close(this->server_fd);
        exit(EXIT_FAILURE);
    }

    // Start Listening for Incoming Connections (backog size = 5)
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Error: Failed to listen on port " << port << "\n";
        close(this->server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server successfully initialized on port " << this->port << ".\n";
}

Server::~Server() {
    // Close the Server Socket when the Object Goes out of Scope
    if (this->server_fd != -1) {
        close(this->server_fd);
        std::cout << "Server shut down.\n";
    }
}

void Server::start() {
    std::cout << "Waiting for incoming connections...\n";

    while (true) {
        // Accept a New Connection
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(this->server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Error: Failed to accept client connection\n";
            continue;
        }
        std::cout << "Client connected\n";

        // Receive Data from the Client and Print It
        receive(client_fd);

        // Close the Client Socket
        close(client_fd);
    }
}

void Server::receive(int client_fd) {
    const int buffer_size = 1024;
    char buffer[buffer_size];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_fd, buffer, buffer_size - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';  // Ensure String is Null-Terminated
        std::cout << "Received: " << buffer << std::endl;
    }

    if (bytes_received < 0) {
        std::cerr << "Error: Failed to receive data from client\n";
    } else {
        std::cout << "Client disconnected\n";
    }
}

bool Server::readFile(std::string_view file_path, std::vector<char>& buffer) {
    std::ifstream file(std::string(file_path), std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    std::streamsize size = file.tellg();
    if (size < 0) {
        return false;
    }

    buffer.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(buffer.data(), size)) {
        return false;
    }

    return true;
}

bool Server::writeFile(std::string_view file_path, const std::vector<char>& buffer) {
    std::ofstream file(std::string(file_path), std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(buffer.data(), buffer.size());
    return file.good();
}