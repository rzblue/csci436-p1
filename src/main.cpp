#include <iostream>
#include <cstring>
#include "FileClient.hpp"
#include "FileServer.hpp"
#include "ProxyServer.hpp"
#include "HTTPProxyServer.hpp"


int main(int argc, char* argv[]) {
    // Basic Argument Parsing
    if (argc < 2) {
        std::cerr << "Usage:\n";
        std::cerr << "  " << argv[0] << " server <port>\n";
        std::cerr << "  " << argv[0] << " client <host> <port> [proxy-host] [proxy-port]\n";
        std::cerr << "  " << argv[0] << " proxy <port>\n";
        std::cerr << "  " << argv[0] << " http-proxy <port>\n";
        return 1;
    }

    // Server Mode
    if (strcmp(argv[1], "server") == 0) {
        int port = (argc >= 3) ? std::stoi(argv[2]) : 5000;
        FileServer server(port);
        server.start();
    }
    
    // Client Mode
    else if (strcmp(argv[1], "client") == 0) {
        if (argc < 4) {
            std::cerr << "Usage: " << argv[0] << " client <host> <port> [proxy-host] [proxy-port]\n";
            return 1;
        }
        
        // Parse Host and Port
        std::string host = argv[2];
        int port;
        try {
            port = std::stoi(argv[3]);
        } catch (const std::invalid_argument&) {
            std::cerr << "Invalid port number: " << argv[3] << "\n";
            return 1;
        }

        // Direct Connection
        if (argc == 4) {
            FileClient client(host, port);
            client.start();
        }
        // Proxy Connection
        else {
            // Parse Proxy Host and Port
            std::string proxy_host = argv[4];
            int proxy_port;
            try {
                proxy_port = std::stoi(argv[5]);
            } catch (const std::invalid_argument&) {
                std::cerr << "Invalid proxy port number: " << argv[5] << "\n";
                return 1;
            }

            FileClient client(host, port, proxy_host, proxy_port);
            client.start();
        }

        
    }

    // Proxy Server Mode
    else if (strcmp(argv[1], "proxy") == 0) {
        int port = (argc >= 3) ? std::stoi(argv[2]) : 5000;
        ProxyServer server(port);
        server.start();
    }

    // HTTP Proxy Server Mode
    else if (strcmp(argv[1], "http-proxy") == 0) {
        int port = (argc >= 3) ? std::stoi(argv[2]) : 8080;
        HTTPProxyServer server(port);
        server.start();
    }

    // Invalid Mode
    else {
        std::cerr << "Unknown mode: " << argv[1] << "\n";
        return 1;
    }

    return 0;
}
