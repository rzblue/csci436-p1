#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include <string>

/**
 * HTTPRequestParser - Handles reading and parsing HTTP requests from client sockets
 * 
 * Responsibilities:
 * - Read complete HTTP request (headers + body if present)
 * - Parse destination host and port
 * - Determine request type (GET, POST, CONNECT, etc.)
 */
class HTTPRequestParser {
public:
    /**
     * Represents a parsed HTTP destination
     */
    struct Destination {
        std::string host;
        int port;
        bool valid;
        
        Destination() : port(80), valid(false) {}
        Destination(const std::string& h, int p) : host(h), port(p), valid(true) {}
    };

    /**
     * Read a complete HTTP request from the client socket
     * 
     * @param client_fd Socket file descriptor
     * @return Complete HTTP request as string (headers + body), or empty string on error
     */
    static std::string readRequest(int client_fd);

    /**
     * Parse the destination host and port from an HTTP request
     * 
     * Handles both:
     * - Standard requests: GET /path HTTP/1.1\r\nHost: example.com\r\n
     * - CONNECT requests: CONNECT example.com:443 HTTP/1.1\r\n
     * 
     * @param request Complete HTTP request string
     * @return Destination object with host, port, and validity flag
     */
    static Destination parseDestination(const std::string& request);

    /**
     * Check if request is a CONNECT method (for HTTPS tunneling)
     * 
     * @param request HTTP request string
     * @return true if this is a CONNECT request
     */
    static bool isConnectRequest(const std::string& request);

    /**
     * Get HTTP method from request
     * 
     * @param request HTTP request string
     * @return Method string (GET, POST, CONNECT, etc.) or empty string on parse error
     */
    static std::string getMethod(const std::string& request);

    /**
     * Extract the value of a specific header (case-insensitive)
     * 
     * @param request HTTP request string
     * @param header_name Name of header to find (e.g., "Content-Length")
     * @return Header value, or empty string if not found
     */
    static std::string getHeader(const std::string& request, const std::string& header_name);

    /**
     * Check if request indicates connection should be kept alive
     * 
     * @param request HTTP request string
     * @return true if connection should persist (HTTP/1.1 default), false otherwise
     */
    static bool shouldKeepAlive(const std::string& request);

    /**
    * Remove a specific header from an HTTP request
    *
    * @param request HTTP request string
    * @param header_name Name of header to remove (case-insensitive)
    * @return Modified request with header removed
    */
    static std::string removeHeader(std::string_view headers, std::string_view header_name);

private:
    /**
     * Helper: Read until we have complete headers (ending with \r\n\r\n)
     * 
     * @param client_fd Socket file descriptor
     * @param headers Output parameter for headers
     * @return true on success, false on connection error
     */
    static bool readHeaders(int client_fd, std::string& headers);

    /**
     * Helper: Read exactly n bytes from socket
     * 
     * @param fd Socket file descriptor
     * @param n Number of bytes to read
     * @return Data read, or shorter string if connection closed
     */
    static std::string readExact(int fd, size_t n);

    /**
     * Helper: Parse Content-Length from headers
     * 
     * @param headers HTTP headers string
     * @return Content length, or 0 if not present
     */
    static size_t parseContentLength(const std::string& headers);
};

#endif // HTTP_REQUEST_PARSER_HPP