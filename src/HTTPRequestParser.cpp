#include "HTTPRequestParser.hpp"
#include <sys/socket.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>
#include "StringUtils.hpp"

using namespace utils;

// ============================================================================
// Public Methods
// ============================================================================

std::string HTTPRequestParser::readRequest(int client_fd) {
    std::string headers;
    if (!readHeaders(client_fd, headers)) {
        return "";  // Connection error
    }

    // Check if request has a body (Content-Length header)
    size_t content_length = parseContentLength(headers);
    
    if (content_length == 0) {
        // No body (typical for GET, HEAD, CONNECT)
        return headers;
    }

    // Calculate how much of the body we already received with headers
    size_t header_end = headers.find("\r\n\r\n");
    size_t already_have = headers.size() - (header_end + 4);

    if (already_have >= content_length) {
        // Already have complete body
        return headers;
    }

    // Read remaining body
    size_t remaining = content_length - already_have;
    std::string body = readExact(client_fd, remaining);
    
    if (body.size() != remaining) {
        std::cerr << "[HTTPRequestParser] Incomplete request body\n";
        return "";
    }

    return headers + body;
}

HTTPRequestParser::Destination HTTPRequestParser::parseDestination(const std::string& request) {
    Destination dest;
    dest.port = 80;  // Default HTTP port

    std::istringstream stream(request);
    std::string method, url, version;
    stream >> method >> url >> version;

    // Case 1: CONNECT request (HTTPS tunnel)
    // Format: CONNECT example.com:443 HTTP/1.1
    if (method == "CONNECT") {
        size_t colon = url.find(':');
        if (colon == std::string::npos) {
            dest.valid = false;
            return dest;
        }

        dest.host = url.substr(0, colon);
        try {
            dest.port = std::stoi(url.substr(colon + 1));
            dest.valid = true;
        } catch (...) {
            dest.valid = false;
        }
        return dest;
    }

    // Case 2: Standard HTTP request
    // Must parse Host header
    std::string host_value = getHeader(request, "Host");
    if (host_value.empty()) {
        dest.valid = false;
        return dest;
    }

    // Check for port in Host header (e.g., "example.com:8080")
    size_t colon = host_value.find(':');
    if (colon != std::string::npos) {
        dest.host = host_value.substr(0, colon);
        try {
            dest.port = std::stoi(host_value.substr(colon + 1));
        } catch (...) {
            dest.port = 80;
        }
    } else {
        dest.host = host_value;
    }

    dest.valid = true;
    return dest;
}

bool HTTPRequestParser::isConnectRequest(const std::string& request) {
    return request.rfind("CONNECT ", 0) == 0;
}

std::string HTTPRequestParser::getMethod(const std::string& request) {
    std::istringstream stream(request);
    std::string method;
    stream >> method;
    return method;
}

std::string HTTPRequestParser::getHeader(const std::string& request, 
                                          const std::string& header_name) {
    // Create lowercase versions for case-insensitive search
    std::string lower_request = toLower(request);
    std::string lower_name = toLower(header_name);
    
    // Find header line
    std::string search_key = lower_name + ":";
    size_t pos = lower_request.find(search_key);
    
    if (pos == std::string::npos) {
        return "";
    }

    // Move past the header name and colon
    pos += search_key.length();

    // Skip whitespace
    while (pos < request.size() && (request[pos] == ' ' || request[pos] == '\t')) {
        pos++;
    }

    // Find end of line
    size_t end = request.find("\r\n", pos);
    if (end == std::string::npos) {
        return "";
    }

    // Extract value from ORIGINAL request (not lowercase version)
    return request.substr(pos, end - pos);
}

bool HTTPRequestParser::shouldKeepAlive(const std::string& request) {
    // Check HTTP version
    bool is_http_1_0 = (request.find("HTTP/1.0") != std::string::npos);
    
    // Get Connection header
    std::string connection = getHeader(request, "Connection");
    std::string lower_conn = toLower(connection);

    // HTTP/1.0: Keep-alive only if explicitly requested
    if (is_http_1_0) {
        return (lower_conn == "keep-alive");
    }

    // HTTP/1.1: Keep-alive by default unless "close" specified
    return (lower_conn != "close");
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool HTTPRequestParser::readHeaders(int client_fd, std::string& headers) {
    headers.clear();
    char buf[8192];

    while (true) {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            return false;  // Connection closed or error
        }

        headers.append(buf, n);

        // Check if we have complete headers
        if (headers.find("\r\n\r\n") != std::string::npos) {
            return true;
        }
    }
}

std::string HTTPRequestParser::readExact(int fd, size_t n) {
    std::string result;
    result.reserve(n);
    
    char buf[8192];
    size_t total = 0;

    while (total < n) {
        size_t to_read = std::min(sizeof(buf), n - total);
        ssize_t bytes = recv(fd, buf, to_read, 0);

        if (bytes <= 0) {
            return result;  // Partial read or error
        }

        result.append(buf, bytes);
        total += bytes;
    }

    return result;
}

size_t HTTPRequestParser::parseContentLength(const std::string& headers) {
    std::string value = getHeader(headers, "Content-Length");
    
    if (value.empty()) {
        return 0;
    }

    try {
        return static_cast<size_t>(std::stoul(value));
    } catch (...) {
        return 0;
    }
}
