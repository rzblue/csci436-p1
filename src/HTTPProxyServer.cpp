#include "HTTPProxyServer.hpp"

#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <algorithm>


void HTTPProxyServer::handleRequest(int client_fd) {
    // Read the Initial Request from the Client
    char buffer[8192];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        std::cerr << "[HTTPProxyServer] Failed to read from client\n";
        return;
    }
    buffer[bytes_read] = '\0';
    std::string request(buffer);

    // TODO: Console Logging: Replace or Supplement with Custom Logging Module
    std::cout << "\n[HTTPProxyServer] Received request:\n"
              << request.substr(0, std::min<size_t>(request.size(), 300))  // limit output
              << (request.size() > 300 ? "...\n" : "\n");

    // Parse Destination Host and Port from the Request
    std::string host;
    int port = 80;
    if (!parseHttpDestination(request, host, port)) {
        std::cerr << "[HTTPProxyServer] Failed to parse destination\n";
        return;
    }

    // Handle HTTPS Tunneling (CONNECT)
    if (request.rfind("CONNECT ", 0) == 0) {
        int remote_fd = connectToHost(host, port);
        if (remote_fd < 0) {
            // Send 502 Bad Gateway if Connection Fails
            std::string response = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
            return;
        }

        // Acknowledge Tunnel Establishment to the Client
        std::string response = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(client_fd, response.c_str(), response.size(), 0);

        // TODO: Console Logging: Replace or Supplement with Custom Logging Module
        std::cout << "[HTTPProxyServer] Tunnel established to " << host << ":" << port << "\n";

        // Relay Bytes between the Client and Server (Bidirectional)
        fd_set fds;
        char relay_buf[4096];
        while (true) {
            FD_ZERO(&fds);
            FD_SET(client_fd, &fds);
            FD_SET(remote_fd, &fds);
            int max_fd = std::max(client_fd, remote_fd) + 1;

            if (select(max_fd, &fds, nullptr, nullptr, nullptr) < 0)
                break;

            // Client → Server
            if (FD_ISSET(client_fd, &fds)) {
                ssize_t n = recv(client_fd, relay_buf, sizeof(relay_buf), 0);
                if (n <= 0) break;
                send(remote_fd, relay_buf, n, 0);
            }

            // Server → Client
            if (FD_ISSET(remote_fd, &fds)) {
                ssize_t n = recv(remote_fd, relay_buf, sizeof(relay_buf), 0);
                if (n <= 0) break;
                send(client_fd, relay_buf, n, 0);
            }
        }

        // TODO: Console Logging: Replace or Supplement with Custom Logging Module
        std::cout << "[HTTPProxyServer] Tunnel closed for " << host << "\n";

        close(remote_fd);
        return;
    }

    // Otherwise, Handle Normal HTTP Request
    int server_fd = connectToHost(host, port);
    if (server_fd < 0) {
        // Send 502 Bad Gateway if Connection Fails
        std::string response = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    // Forward the Client's Request to the Remote Host
    send(server_fd, request.c_str(), request.size(), 0);

    // Relay the Response from the Server Back to the Client
    ssize_t n;
    while ((n = recv(server_fd, buffer, sizeof(buffer), 0)) > 0) {
        send(client_fd, buffer, n, 0);
    }

    // TODO: Console Logging: Replace or Supplement with Custom Logging Module
    std::cout << "[HTTPProxyServer] Finished relaying response from " << host << "\n";

    close(server_fd);
}


bool HTTPProxyServer::parseHttpDestination(const std::string& request, std::string& out_host, int& out_port) {
    std::istringstream stream(request);
    std::string method, url, version;
    stream >> method >> url >> version;

    // Handle HTTPS CONNECT Requests
    // Format: "CONNECT <host>:<port> HTTP/1.1"
    if (method == "CONNECT") {
        size_t colon = url.find(':');
        if (colon == std::string::npos) return false;
        out_host = url.substr(0, colon);
        out_port = std::stoi(url.substr(colon + 1));
        return true;
    }

    // Handle Standard HTTP Requests (GET, POST, etc.)
    // Look for Host Header in the Request
    size_t host_pos = request.find("Host:");
    if (host_pos == std::string::npos) return false;

    // Extract the Host Line
    size_t host_start = request.find_first_not_of(" \t", host_pos + 5);
    size_t host_end = request.find("\r\n", host_start);
    if (host_start == std::string::npos || host_end == std::string::npos)
        return false;
    std::string host_line = request.substr(host_start, host_end - host_start);

    // Split host:port if Present
    size_t colon = host_line.find(':');
    if (colon != std::string::npos) {
        out_host = host_line.substr(0, colon);
        out_port = std::stoi(host_line.substr(colon + 1));
    } else {
        out_host = host_line;
        out_port = 80;
    }

    return true;
}


int HTTPProxyServer::connectToHost(const std::string& host, int port) {
    // Prepare Hints Struct for getaddrinfo
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Resolve Hostname to an IP Address
    std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
        std::cerr << "[HTTPProxyServer] DNS resolution failed for " << host << "\n";
        return -1;
    }

    // Create a Socket using the Resolved Address Info
    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd < 0) {
        std::cerr << "[HTTPProxyServer] Failed to create socket for " << host << "\n";
        freeaddrinfo(res);
        return -1;
    }

    // Attempt to Connect to the Remote Host
    if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "[HTTPProxyServer] Failed to connect to " << host << ":" << port << "\n";
        close(sock_fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock_fd;
}
