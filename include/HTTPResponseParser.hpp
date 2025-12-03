#ifndef HTTP_RESPONSE_PARSER_HPP
#define HTTP_RESPONSE_PARSER_HPP

#include <string>

/**
 * HTTPResponseParser - Handles reading and parsing HTTP responses from server sockets
 * 
 * Responsibilities:
 * - Read complete HTTP response (headers + body)
 * - Handle different body encoding methods:
 *   - Content-Length (fixed size)
 *   - Transfer-Encoding: chunked
 *   - Connection close (read until EOF)
 * - Parse response status and headers
 */
class HTTPResponseParser {
public:
    /**
     * Represents a parsed HTTP response
     */
    struct ParsedResponse {
        std::string headers;      // Complete headers (including \r\n\r\n)
        std::string body;         // Complete body
        std::string full;         // headers + body
        int status_code;          // HTTP status code (200, 404, etc.)
        bool valid;               // Whether parsing succeeded
        
        ParsedResponse() : status_code(0), valid(false) {}
    };

    /**
     * Read a complete HTTP response from the server socket
     * 
     * This handles all three methods of specifying body length:
     * 1. Content-Length header
     * 2. Transfer-Encoding: chunked
     * 3. Read until connection closes (HTTP/1.0 style)
     * 
     * @param server_fd Socket file descriptor
     * @return ParsedResponse object with headers, body, and metadata
     */
    static ParsedResponse readResponse(int server_fd);

    /**
     * Extract the value of a specific header (case-insensitive)
     * 
     * @param headers HTTP headers string
     * @param header_name Name of header to find
     * @return Header value, or empty string if not found
     */
    static std::string getHeader(const std::string& headers, const std::string& header_name);

    /**
     * Check if response indicates connection should be kept alive
     * 
     * @param headers HTTP headers string
     * @return true if connection should persist, false otherwise
     */
    static bool shouldKeepAlive(const std::string& headers);

    /**
     * Extract status code from response
     * 
     * @param headers HTTP headers string (or full response)
     * @return Status code (200, 404, etc.) or 0 on parse error
     */
    static int getStatusCode(const std::string& headers);

private:
    /**
     * Read headers with overflow handling
     * Returns headers and any extra bytes that were read (part of body)
     */
    static bool readHeaders(int fd, std::string& headers, std::string& overflow);

    /**
     * Check if response should have no body based on status code
     * (1xx informational, 204 No Content, 304 Not Modified)
     */
    static bool shouldHaveNoBody(int status_code);

    /**
     * Check if response uses chunked transfer encoding
     */
    static bool isChunked(const std::string& headers);

    /**
     * Parse Content-Length header value
     */
    static size_t parseContentLength(const std::string& headers);

    /**
     * Read fixed-length body (when Content-Length is specified)
     */
    static std::string readFixedLengthBody(int fd, size_t length, const std::string& overflow);

    /**
     * Read chunked-encoded body
     * Format: <hex-size>\r\n<data>\r\n ... 0\r\n\r\n
     */
    static std::string readChunkedBody(int fd, std::string& buffer);

    /**
     * Read body until connection closes (HTTP/1.0 style)
     */
    static std::string readUntilClose(int fd, const std::string& overflow);

    // Buffered reading helpers for chunked encoding
    static bool readLine(int fd, std::string& buffer, std::string& line);
    static std::string readExactFromBuffer(int fd, std::string& buffer, size_t n);

    /**
     * Helper: Convert string to lowercase
     */
    static std::string toLower(const std::string& str);
};

#endif // HTTP_RESPONSE_PARSER_HPP