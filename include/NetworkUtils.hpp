#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

#include <string>

/**
 * NetworkUtils - Common networking utility functions
 * 
 * Provides reusable networking operations used across multiple components:
 * - Making outbound TCP connections
 * - Sending data with error checking
 * - Receiving data with timeout/error handling
 * 
 * This is a utility class with static methods only.
 */
class NetworkUtils {
public:
    /**
     * Connect to a remote host
     * 
     * Performs DNS resolution and establishes a TCP connection.
     * Supports both IPv4 and IPv6.
     * 
     * @param host Hostname or IP address
     * @param port Port number
     * @return Socket file descriptor on success, -1 on failure
     */
    static int connectToHost(const std::string& host, int port);

    /**
     * Send complete data to socket
     * 
     * Ensures all data is sent or returns error.
     * Handles partial sends automatically.
     * 
     * @param fd Socket file descriptor
     * @param data Data to send
     * @param length Length of data in bytes
     * @return true on success, false on failure
     */
    static bool sendData(int fd, const char* data, size_t length);

    /**
     * Send string data to socket
     * 
     * Convenience wrapper for sending std::string data.
     * 
     * @param fd Socket file descriptor
     * @param data String data to send
     * @return true on success, false on failure
     */
    static bool sendData(int fd, const std::string& data);

    /**
     * Receive up to max_length bytes from socket
     * 
     * Non-blocking read that returns whatever data is available.
     * 
     * @param fd Socket file descriptor
     * @param buffer Buffer to store received data
     * @param max_length Maximum bytes to receive
     * @return Number of bytes received, 0 on EOF, -1 on error
     */
    static ssize_t receiveData(int fd, char* buffer, size_t max_length);

    /**
     * Set socket timeout
     * 
     * Sets both send and receive timeouts.
     * 
     * @param fd Socket file descriptor
     * @param seconds Timeout in seconds
     * @return true on success, false on failure
     */
    static bool setSocketTimeout(int fd, int seconds);

    /**
     * Get last socket error as string
     * 
     * @return Human-readable error message
     */
    static std::string getLastError();

private:
    // Utility class - no instances allowed
    NetworkUtils() = delete;
    ~NetworkUtils() = delete;
    NetworkUtils(const NetworkUtils&) = delete;
    NetworkUtils& operator=(const NetworkUtils&) = delete;
};

#endif // NETWORK_UTILS_HPP