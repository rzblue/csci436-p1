// ============================================================================
// HTTPProxyServer.hpp - Refactored Version
// ============================================================================
#ifndef HTTP_PROXY_SERVER_HPP
#define HTTP_PROXY_SERVER_HPP

#include "BaseServer.hpp"
#include <string>
#include <vector>

/**
 * HTTPProxyServer - Multi-threaded HTTP/HTTPS proxy with content filtering
 * 
 * Features:
 * - HTTP proxying (GET, POST, etc.)
 * - HTTPS tunneling (CONNECT method)
 * - Request and response filtering based on forbidden words
 * - Persistent connections (HTTP/1.1 keep-alive)
 */
class HTTPProxyServer : public BaseServer {
public:
    /**
     * Constructor
     * @param port Port to listen on
     */
    explicit HTTPProxyServer(int port);

protected:
    /**
     * Handle a client connection
     * This is called by BaseServer for each accepted connection
     * 
     * @param client_fd Client socket file descriptor
     */
    void handleRequest(int client_fd) override;

private:
    // === Forbidden Word Management ===
    std::vector<std::string> forbidden_words;
    
    /**
     * Load forbidden words from file
     * @return true on success, false on failure
     */
    bool loadForbiddenWords();
    
    /**
     * Check if text contains forbidden words
     * @param text Text to check
     * @param matches Output vector of matched words
     * @return true if forbidden words found
     */
    bool containsForbiddenWords(const std::string& text, 
                                std::vector<std::string>& matches) const;

    // === HTTPS Tunnel Handling ===
    
    /**
     * Handle CONNECT request (HTTPS tunnel)
     * @param client_fd Client socket
     * @param host Destination host
     * @param port Destination port
     */
    void handleConnectTunnel(int client_fd, 
                             const std::string& host, 
                             int port);

    // === Networking Utilities ===
    
    /**
     * Connect to a remote host
     * @param host Hostname or IP address
     * @param port Port number
     * @return Socket file descriptor, or -1 on failure
     */
    int connectToHost(const std::string& host, int port);
    
    /**
     * Send data to socket with error checking
     * @param fd Socket file descriptor
     * @param data Data to send
     * @return true on success, false on failure
     */
    bool sendData(int fd, const std::string& data);
};

#endif // HTTP_PROXY_SERVER_HPP
