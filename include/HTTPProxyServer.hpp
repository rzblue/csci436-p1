#ifndef HTTP_PROXY_SERVER_HPP
#define HTTP_PROXY_SERVER_HPP

#include "BaseServer.hpp"
#include <string>
#include <netinet/in.h>

/**
 * @brief A simple multithreaded HTTP proxy server.
 * 
 * The HTTPProxyServer acts as an intermediary between an HTTP client
 * (such as a web browser) and remote web servers. It parses client
 * HTTP requests, establishes connections to the appropriate servers,
 * forwards requests, and relays responses back to the client.
 * 
 * Supported:
 *  - HTTP methods: GET, POST, etc. (standard requests)
 *  - CONNECT method for HTTPS tunneling
 * 
 * Logging and multi-threading are handled by BaseServer.
 */
class HTTPProxyServer : public BaseServer {
public:
    using BaseServer::BaseServer; // Inherit constructor

protected:
    /**
     * @brief Handles a single client connection.
     * 
     * Reads the client's HTTP request, determines the target host
     * and port, connects to the destination server, forwards the
     * request, and relays responses between the client and server.
     */
    void handleRequest(int client_fd) override;

private:
    /**
     * @brief Parses the "Host" header or URL in the HTTP request line.
     * 
     * @param request The full HTTP request text from the client.
     * @param out_host Receives the destination hostname.
     * @param out_port Receives the destination port (defaults to 80 if not specified).
     * @return true if parsing succeeded, false otherwise.
     */
    bool parseHttpDestination(const std::string& request, std::string& out_host, int& out_port);

    /**
     * @brief Establishes a TCP connection to the specified host and port.
     * 
     * @param host Destination hostname or IP address.
     * @param port Destination port number.
     * @return Socket file descriptor on success, or -1 on failure.
     */
    int connectToHost(const std::string& host, int port);
};

#endif // HTTP_PROXY_SERVER_HPP
