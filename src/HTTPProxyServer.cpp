#include "HTTPProxyServer.hpp"
#include "HTTPRequestParser.hpp"
#include "HTTPResponseParser.hpp"
#include "ErrorResponseBuilder.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <iostream>

// ====================================================================================================
// Constructor
// ====================================================================================================
HTTPProxyServer::HTTPProxyServer(int port, const std::string& forbidden_file) 
    : BaseServer(port), filter(forbidden_file) {
    
    if (filter.isEmpty()) {
        std::cerr << "[HTTPProxyServer] Warning: No forbidden words loaded.\n";
    }
}

// ====================================================================================================
// Main Request Handler
// ====================================================================================================
void HTTPProxyServer::handleRequest(int client_fd) {
    while (true) {
        // Read and parse client request
        std::string request = HTTPRequestParser::readRequest(client_fd);
        if (request.empty()) {
            return;  // Client disconnected
        }

        std::cout << "[Proxy] Received " 
                  << HTTPRequestParser::getMethod(request) 
                  << " request\n";

        // Check for forbidden words in request
        std::vector<std::string> matches;
        if (filter.containsForbiddenContent(request, matches)) {
            std::cout << "[Proxy] Request blocked (forbidden content)\n";
            sendData(client_fd, ErrorResponseBuilder::build403Forbidden(matches));
            return;
        }

        // Parse destination host and port
        auto dest = HTTPRequestParser::parseDestination(request);
        if (!dest.valid) {
            std::cerr << "[Proxy] Could not parse destination\n";
            sendData(client_fd, ErrorResponseBuilder::build400BadRequest("Invalid destination"));
            return;
        }

        std::cout << "[Proxy] Destination: " << dest.host << ":" << dest.port << "\n";

        // Handle CONNECT method (HTTPS tunnel)
        if (HTTPRequestParser::isConnectRequest(request)) {
            handleConnectTunnel(client_fd, dest.host, dest.port);
            return;
        }

        // Connect to destination server
        int server_fd = connectToHost(dest.host, dest.port);
        if (server_fd < 0) {
            std::cerr << "[Proxy] Failed to connect to " << dest.host << "\n";
            sendData(client_fd, ErrorResponseBuilder::build502BadGateway(
                "Could not connect to " + dest.host));
            return;
        }

        // Forward request to server
        if (!sendData(server_fd, request)) {
            std::cerr << "[Proxy] Failed to send request to server\n";
            close(server_fd);
            return;
        }

        // Read and parse server response
        auto response = HTTPResponseParser::readResponse(server_fd);
        
        if (!response.valid || response.full.empty()) {
            std::cerr << "[Proxy] Invalid or empty response from server\n";
            close(server_fd);
            return;
        }

        std::cout << "[Proxy] Response status: " << response.status_code << "\n";

        // Check for forbidden words in response body
        matches.clear();
        if (filter.containsForbiddenContent(response.body, matches)) {
            std::cout << "[Proxy] Response blocked (forbidden content)\n";
            sendData(client_fd, ErrorResponseBuilder::build503ServiceUnavailable(matches));
            close(server_fd);
            return;
        }

        // Forward clean response to client
        if (!sendData(client_fd, response.full)) {
            std::cerr << "[Proxy] Failed to send response to client\n";
            close(server_fd);
            return;
        }

        std::cout << "[Proxy] Response forwarded successfully\n";

        // Determine if connection should persist
        bool request_keep_alive = HTTPRequestParser::shouldKeepAlive(request);
        bool response_keep_alive = HTTPResponseParser::shouldKeepAlive(response.headers);

        close(server_fd);  // Always close server connection

        if (!request_keep_alive || !response_keep_alive) {
            std::cout << "[Proxy] Closing connection\n";
            return;
        }

        std::cout << "[Proxy] Keeping connection alive for next request\n";
        // Loop continues to handle next request from same client
    }
}

// ====================================================================================================
// HTTPS Tunnel Handler
// ====================================================================================================
void HTTPProxyServer::handleConnectTunnel(int client_fd, 
                                          const std::string& host, 
                                          int port) {
    std::cout << "[Proxy] Establishing HTTPS tunnel to " << host << ":" << port << "\n";

    // Connect to destination server
    int server_fd = connectToHost(host, port);
    if (server_fd < 0) {
        std::cerr << "[Proxy] Tunnel connection failed\n";
        sendData(client_fd, ErrorResponseBuilder::build502BadGateway(
            "Could not establish tunnel to " + host));
        return;
    }

    // Inform client that tunnel is established
    if (!sendData(client_fd, "HTTP/1.1 200 Connection Established\r\n\r\n")) {
        close(server_fd);
        return;
    }

    std::cout << "[Proxy] Tunnel established, forwarding traffic...\n";

    // Bidirectional forwarding loop
    fd_set read_fds;
    char buf[8192];

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = std::max(client_fd, server_fd) + 1;

        int activity = select(max_fd, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            std::cerr << "[Proxy] Select error in tunnel\n";
            break;
        }

        // Client -> Server
        if (FD_ISSET(client_fd, &read_fds)) {
            ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
            if (n <= 0) {
                std::cout << "[Proxy] Client closed tunnel\n";
                break;
            }
            
            if (send(server_fd, buf, n, 0) <= 0) {
                std::cerr << "[Proxy] Failed to forward client data\n";
                break;
            }
        }

        // Server -> Client
        if (FD_ISSET(server_fd, &read_fds)) {
            ssize_t n = recv(server_fd, buf, sizeof(buf), 0);
            if (n <= 0) {
                std::cout << "[Proxy] Server closed tunnel\n";
                break;
            }
            
            if (send(client_fd, buf, n, 0) <= 0) {
                std::cerr << "[Proxy] Failed to forward server data\n";
                break;
            }
        }
    }

    close(server_fd);
    std::cout << "[Proxy] Tunnel closed\n";
}

// ====================================================================================================
// Networking Utilities
// ====================================================================================================
int HTTPProxyServer::connectToHost(const std::string& host, int port) {
    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP

    struct addrinfo* res = nullptr;
    std::string port_str = std::to_string(port);

    int err = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (err != 0) {
        std::cerr << "[Proxy] DNS resolution failed for " << host
                  << ": " << gai_strerror(err) << "\n";
        return -1;
    }

    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd < 0) {
        std::cerr << "[Proxy] Failed to create socket\n";
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "[Proxy] Failed to connect to " << host << ":" << port
                  << " (" << strerror(errno) << ")\n";
        close(sock_fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock_fd;
}

bool HTTPProxyServer::sendData(int fd, const std::string& data) {
    ssize_t sent = send(fd, data.c_str(), data.size(), 0);
    if (sent < 0) {
        std::cerr << "[Proxy] Send failed: " << strerror(errno) << "\n";
        return false;
    }
    if (static_cast<size_t>(sent) != data.size()) {
        std::cerr << "[Proxy] Partial send: " << sent << "/" << data.size() << "\n";
        return false;
    }
    return true;
}