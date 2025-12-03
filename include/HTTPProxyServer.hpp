#ifndef HTTP_PROXY_SERVER_HPP
#define HTTP_PROXY_SERVER_HPP

#include "BaseServer.hpp"
#include "ContentFilter.hpp"
#include <string>

/**
 * HTTPProxyServer - Multi-threaded HTTP/HTTPS proxy with content filtering
 * 
 * Features:
 * - HTTP proxying (GET, POST, etc.)
 * - HTTPS tunneling (CONNECT method)
 * - Request and response filtering based on forbidden words
 * - Persistent connections (HTTP/1.1 keep-alive)
 * 
 * This class orchestrates the proxy workflow by delegating to specialized components:
 * - HTTPRequestParser: Parses incoming HTTP requests
 * - HTTPResponseParser: Parses server HTTP responses
 * - ContentFilter: Checks for forbidden content
 * - ErrorResponseBuilder: Generates error pages
 */
class HTTPProxyServer : public BaseServer {
public:
    /**
     * Constructor
     * @param port Port to listen on
     * @param forbidden_file Path to forbidden words file (default: "forbidden.txt")
     */
    explicit HTTPProxyServer(int port, const std::string& forbidden_file = "forbidden.txt");

protected:
    /**
     * Handle a client connection
     * This is called by BaseServer for each accepted connection
     * 
     * @param client_fd Client socket file descriptor
     */
    void handleRequest(int client_fd) override;

private:
    // Content filtering component    
    ContentFilter filter;

    /**
     * Handle CONNECT request (HTTPS tunnel)
     * Establishes a bidirectional tunnel between client and server
     * 
     * @param client_fd Client socket
     * @param host Destination host
     * @param port Destination port
     */
    void handleConnectTunnel(int client_fd, 
                             const std::string& host, 
                             int port);

    /**
     * Connect to a remote host
     * 
     * @param host Hostname or IP address
     * @param port Port number
     * @return Socket file descriptor, or -1 on failure
     */
    int connectToHost(const std::string& host, int port);
    
    /**
     * Send data to socket with error checking
     * 
     * @param fd Socket file descriptor
     * @param data Data to send
     * @return true on success, false on failure
     */
    bool sendData(int fd, const std::string& data);
};

#endif // HTTP_PROXY_SERVER_HPP