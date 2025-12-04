#include "HTTPResponseParser.hpp"
#include <sys/socket.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include "StringUtils.hpp"

using namespace utils;

// ============================================================================
// Public Methods
// ============================================================================

HTTPResponseParser::ParsedResponse HTTPResponseParser::readResponse(int server_fd) {
    ParsedResponse response;
    
    // Step 1: Read headers with overflow handling
    std::string overflow;
    if (!readHeaders(server_fd, response.headers, overflow)) {
        std::cerr << "[HTTPResponseParser] Failed to read headers\n";
        return response;
    }

    // Step 2: Extract status code
    response.status_code = getStatusCode(response.headers);

    // Step 3: Check if response should have no body
    if (shouldHaveNoBody(response.status_code)) {
        response.body = "";
        response.full = response.headers;
        response.valid = true;
        return response;
    }

    // Step 4: Determine body reading strategy

    // Case A: Chunked Transfer-Encoding
    if (isChunked(response.headers)) {
        response.body = readChunkedBody(server_fd, overflow);
        response.full = response.headers + response.body;
        response.valid = true;
        return response;
    }

    // Case B: Content-Length specified
    size_t content_length = parseContentLength(response.headers);
    if (content_length > 0) {
        response.body = readFixedLengthBody(server_fd, content_length, overflow);
        response.full = response.headers + response.body;
        response.valid = true;
        return response;
    }

    // Case C: Read until connection closes (HTTP/1.0 style)
    response.body = readUntilClose(server_fd, overflow);
    response.full = response.headers + response.body;
    response.valid = true;
    return response;
}

std::string HTTPResponseParser::getHeader(const std::string& headers, 
                                           const std::string& header_name) {
    // Case-insensitive search
    std::string lower_headers = toLower(headers);
    std::string lower_name = toLower(header_name);
    
    std::string search_key = lower_name + ":";
    size_t pos = lower_headers.find(search_key);
    
    if (pos == std::string::npos) {
        return "";
    }

    pos += search_key.length();

    // Skip whitespace
    while (pos < headers.size() && (headers[pos] == ' ' || headers[pos] == '\t')) {
        pos++;
    }

    // Find end of line
    size_t end = headers.find("\r\n", pos);
    if (end == std::string::npos) {
        return "";
    }

    return headers.substr(pos, end - pos);
}

bool HTTPResponseParser::shouldKeepAlive(const std::string& headers) {
    std::string connection = getHeader(headers, "Connection");
    std::string lower_conn = toLower(connection);
    
    // Check HTTP version from status line
    bool is_http_1_0 = (headers.find("HTTP/1.0") != std::string::npos);
    
    if (is_http_1_0) {
        return (lower_conn == "keep-alive");
    }
    
    // HTTP/1.1: default is keep-alive unless "close" specified
    return (lower_conn != "close");
}

int HTTPResponseParser::getStatusCode(const std::string& headers) {
    // Parse status line: "HTTP/1.1 200 OK\r\n"
    size_t space1 = headers.find(' ');
    if (space1 == std::string::npos) {
        return 0;
    }
    
    size_t space2 = headers.find(' ', space1 + 1);
    if (space2 == std::string::npos) {
        return 0;
    }
    
    std::string status_str = headers.substr(space1 + 1, space2 - space1 - 1);
    
    try {
        return std::stoi(status_str);
    } catch (...) {
        return 0;
    }
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool HTTPResponseParser::readHeaders(int fd, std::string& headers, std::string& overflow) {
    headers.clear();
    overflow.clear();
    
    char buf[4096];
    std::string accumulated;

    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            return false;
        }

        accumulated.append(buf, n);

        size_t header_end = accumulated.find("\r\n\r\n");
        
        if (header_end != std::string::npos) {
            headers = accumulated.substr(0, header_end + 4);
            overflow = accumulated.substr(header_end + 4);
            return true;
        }
    }
}

bool HTTPResponseParser::shouldHaveNoBody(int status_code) {
    // Responses that MUST NOT have a body:
    // - 1xx (Informational)
    // - 204 (No Content)
    // - 304 (Not Modified)
    return (status_code >= 100 && status_code < 200) ||
           status_code == 204 ||
           status_code == 304;
}

bool HTTPResponseParser::isChunked(const std::string& headers) {
    std::string transfer_encoding = getHeader(headers, "Transfer-Encoding");
    std::string lower = toLower(transfer_encoding);
    return (lower.find("chunked") != std::string::npos);
}

size_t HTTPResponseParser::parseContentLength(const std::string& headers) {
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

std::string HTTPResponseParser::readFixedLengthBody(int fd, size_t length, 
                                                     const std::string& overflow) {
    std::string body = overflow;
    
    if (body.size() >= length) {
        return body.substr(0, length);
    }

    body.reserve(length);
    char buf[8192];
    size_t total = body.size();

    while (total < length) {
        size_t to_read = std::min(sizeof(buf), length - total);
        ssize_t n = recv(fd, buf, to_read, 0);
        
        if (n <= 0) {
            std::cerr << "[HTTPResponseParser] Premature EOF ("
                      << total << "/" << length << " bytes)\n";
            break;
        }

        body.append(buf, n);
        total += n;
    }

    return body;
}

std::string HTTPResponseParser::readChunkedBody(int fd, std::string& buffer) {
    std::string body;
    
    while (true) {
        // Step 1: Read chunk size line
        std::string chunk_size_line;
        if (!readLine(fd, buffer, chunk_size_line)) {
            std::cerr << "[HTTPResponseParser] Failed to read chunk size\n";
            return body;
        }

        // Strip chunk extensions (e.g., "1A;foo=bar" -> "1A")
        size_t semicolon = chunk_size_line.find(';');
        if (semicolon != std::string::npos) {
            chunk_size_line = chunk_size_line.substr(0, semicolon);
        }

        // Step 2: Parse hex chunk size
        size_t chunk_size = 0;
        try {
            chunk_size = std::stoul(chunk_size_line, nullptr, 16);
        } catch (...) {
            std::cerr << "[HTTPResponseParser] Invalid chunk size: '" 
                      << chunk_size_line << "'\n";
            return body;
        }

        // Step 3: Check for last chunk (size = 0)
        if (chunk_size == 0) {
            // Read final CRLF and any trailers
            std::string trailing;
            readLine(fd, buffer, trailing);
            return body;
        }

        // Step 4: Read chunk data
        std::string chunk_data = readExactFromBuffer(fd, buffer, chunk_size);
        if (chunk_data.size() != chunk_size) {
            std::cerr << "[HTTPResponseParser] Incomplete chunk data\n";
            return body;
        }
        body += chunk_data;

        // Step 5: Read trailing CRLF
        std::string trailing_crlf;
        readLine(fd, buffer, trailing_crlf);
    }

    return body;
}

std::string HTTPResponseParser::readUntilClose(int fd, const std::string& overflow) {
    std::string body = overflow;
    char buf[8192];

    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        body.append(buf, n);
    }

    return body;
}

// ============================================================================
// Buffered Reading Helpers
// ============================================================================

bool HTTPResponseParser::readLine(int fd, std::string& buffer, std::string& line) {
    line.clear();
    
    while (true) {
        // Check if we have a complete line in buffer
        size_t pos = buffer.find("\r\n");
        if (pos != std::string::npos) {
            line = buffer.substr(0, pos);
            buffer.erase(0, pos + 2);
            return true;
        }

        // Need more data
        char buf[4096];
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            return false;
        }
        buffer.append(buf, n);
    }
}

std::string HTTPResponseParser::readExactFromBuffer(int fd, std::string& buffer, size_t n) {
    // First, use what's in buffer
    if (buffer.size() >= n) {
        std::string result = buffer.substr(0, n);
        buffer.erase(0, n);
        return result;
    }

    // Need more data
    std::string result = buffer;
    buffer.clear();
    
    size_t remaining = n - result.size();
    char buf[8192];

    while (remaining > 0) {
        size_t to_read = std::min(sizeof(buf), remaining);
        ssize_t bytes = recv(fd, buf, to_read, 0);
        if (bytes <= 0) {
            return result;  // Partial read
        }
        result.append(buf, bytes);
        remaining -= bytes;
    }

    return result;
}
