#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "BaseServer.hpp"


BaseServer::BaseServer(int port)
    : socket_fd{-1}, server_port{port} {}

BaseServer::~BaseServer() {
    if (socket_fd != -1) {
        close(socket_fd);
        std::cout << "Server shut down.\n";
    }
}

bool BaseServer::start() {
    // Create a TCP Socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Error: Failed to create socket\n";
        return false;
    }

    // Set Socket Options to Allow Reuse of Address
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: Failed to set socket options\n";
        close(socket_fd);
        return false;
    }

    // Prepare the sockaddr_in Structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    // Bind the Socket to the Port
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Failed to bind to port " << server_port << "\n";
        close(socket_fd);
        return false;
    }

    // Start Listening for Incoming Connections
    if (listen(socket_fd, SOMAXCONN) < 0) {
        std::cerr << "Error: Failed to listen on port " << server_port << "\n";
        close(socket_fd);
        return false;
    }

    // Accept Incoming Connections
    std::cout << "Server listening on port " << server_port << ".\n";
    while (true) {
        int client_fd = acceptConnection();
        if (client_fd < 0) {
            continue; // Accept failed, try again
        }
        std::cout << "Client connected.\n";

        // Create a New Thread for Each Client
        auto* args = new std::pair<BaseServer*, int>(this, client_fd);
        pthread_t thread_id;
        if (pthread_create(&thread_id, nullptr, BaseServer::threadEntry, args) != 0) {
            std::cerr << "Error: Failed to create thread\n";
            close(client_fd);
            delete args;
            continue;
        }

        pthread_detach(thread_id); // Auto-clean threads
    }

    return true;
}

int BaseServer::acceptConnection() {
    // Prepare to Accept a Connection
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    // Accept the Incoming Connection
    int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        std::cerr << "Error: Failed to accept connection\n";
        return -1;
    }
    return client_fd;
}

void* BaseServer::threadEntry(void* arg) {
    auto* args = reinterpret_cast<std::pair<BaseServer*, int>*>(arg);
    BaseServer* server = args->first;
    int client_fd = args->second;
    delete args;

    server->threadHandler(client_fd);

    pthread_exit(nullptr);
    return nullptr;
}

void BaseServer::threadHandler(int client_fd) {
    handleRequest(client_fd);   // accessible (same class)
    close(client_fd);
    std::cout << "Client disconnected.\n";
}