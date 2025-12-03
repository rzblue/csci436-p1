#ifndef HTTPS_TUNNEL_HPP
#define HTTPS_TUNNEL_HPP

#include <string>

/**
 * HTTPSTunnel - Handles HTTPS CONNECT tunneling
 * 
 * Responsibilities:
 * - Establish tunnel connection to destination server
 * - Send tunnel establishment response to client
 * - Perform bidirectional data forwarding between client and server
 * - Handle tunnel cleanup and error conditions
 * 
 * The CONNECT method creates a TCP tunnel through the proxy,
 * primarily used for HTTPS connections where the proxy cannot
 * decrypt or inspect the traffic.
 * 
 * Uses NetworkUtils for connection establishment and data transmission.
 */
class HTTPSTunnel {
public:
    /**
     * Establish and manage an HTTPS tunnel
     * 
     * This method:
     * 1. Connects to the destination server (via NetworkUtils)
     * 2. Sends "200 Connection Established" to client
     * 3. Forwards data bidirectionally until either side closes
     * 
     * @param client_fd Client socket file descriptor
     * @param host Destination hostname
     * @param port Destination port (typically 443 for HTTPS)
     * @return true if tunnel was established successfully, false on connection failure
     */
    static bool establish(int client_fd, const std::string& host, int port);

    /**
     * Establish tunnel with custom success response
     * 
     * Allows sending a custom response to the client after tunnel establishment.
     * 
     * @param client_fd Client socket file descriptor
     * @param host Destination hostname
     * @param port Destination port
     * @param success_response Response to send to client (default: "HTTP/1.1 200 Connection Established\r\n\r\n")
     * @return true if tunnel was established successfully, false on connection failure
     */
    static bool establish(int client_fd, 
                         const std::string& host, 
                         int port,
                         const std::string& success_response);

private:
    /**
     * Perform bidirectional forwarding between client and server
     * 
     * Uses select() for efficient multiplexing.
     * Continues until either side closes the connection.
     * 
     * @param client_fd Client socket
     * @param server_fd Server socket
     */
    static void forwardTraffic(int client_fd, int server_fd);
};

#endif // HTTPS_TUNNEL_HPP