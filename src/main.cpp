#include <iostream>
#include <cstring>
#include "FileClient.hpp"
#include "FileServer.hpp"

int main(int argc, char* argv[]) {
    // Basic Argument Parsing
    if (argc < 2) {
        std::cerr << "Usage:\n";
        std::cerr << "  " << argv[0] << " server <port>\n";
        std::cerr << "  " << argv[0] << " client <host> <port>\n";
        return 1;
    }

    // Determine Mode: Server or Client
    // Server Mode
    if (strcmp(argv[1], "server") == 0) {
        int port = (argc >= 3) ? std::stoi(argv[2]) : 5000;
        FileServer server(port);
        server.start();
    }
    
    // Client Mode
    else if (strcmp(argv[1], "client") == 0) {
        if (argc < 4) {
            std::cerr << "Usage: " << argv[0] << " client <host> <port>\n";
            return 1;
        }

        std::string host = argv[2];
        int port;
        try {
            port = std::stoi(argv[3]);
        } catch (const std::invalid_argument&) {
            std::cerr << "Invalid port number: " << argv[3] << "\n";
            return 1;
        }

        FileClient client(host, port);
        client.start();
    }

    // Invalid Mode
    else {
        std::cerr << "Unknown mode: " << argv[1] << "\n";
        return 1;
    }

    return 0;
}
