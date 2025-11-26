#include "HTTPProxyServer.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

static std::string recvExact(int fd, size_t n);


// ====================================================================================================
// Main Request Handler
// ====================================================================================================
void HTTPProxyServer::handleRequest(int client_fd) {
    // Load forbidden words once per thread if not already loaded
    // (or move this into constructor/startup code)
    if (forbidden_words.empty()) {
        loadForbiddenWords();
    }

    while (true) {
        // -----------------------------------------------------------
        // Step 1: Read full client HTTP request
        // -----------------------------------------------------------
        std::string request = readHttpRequest(client_fd);
        if (request.empty()) {
            // Client disconnected
            return;
        }

        // -----------------------------------------------------------
        // Step 2: Scan request for forbidden words
        // -----------------------------------------------------------
        std::vector<std::string> matches;
        if (containsForbiddenWords(request, matches)) {
            std::string resp = make403Response(matches);
            send(client_fd, resp.c_str(), resp.size(), 0);
            return;  // close connection after sending
        }

        // -----------------------------------------------------------
        // Step 3: Parse Host + Port from request
        // -----------------------------------------------------------
        std::string host;
        int port;

        if (!parseHttpDestination(request, host, port)) {
            std::cerr << "[HTTPProxyServer] Could not parse destination.\n";
            return;
        }

        // Special Case: CONNECT method (HTTPS tunnel)
        if (request.rfind("CONNECT ", 0) == 0) {
            // -----------------------------------------
            // Establish tunnel to remote server
            // -----------------------------------------
            int server_fd = connectToHost(host, port);
            if (server_fd < 0) {
                std::string resp = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
                send(client_fd, resp.c_str(), resp.size(), 0);
                return;
            }

            // Inform client that tunnel is established
            std::string ok = "HTTP/1.1 200 Connection Established\r\n\r\n";
            send(client_fd, ok.c_str(), ok.size(), 0);

            // -----------------------------------------
            // Full-duplex tunnel loop
            // -----------------------------------------
            fd_set fds;
            char buf[4096];

            while (true) {
                FD_ZERO(&fds);
                FD_SET(client_fd, &fds);
                FD_SET(server_fd, &fds);
                int maxfd = std::max(client_fd, server_fd) + 1;

                if (select(maxfd, &fds, nullptr, nullptr, nullptr) < 0) {
                    break;
                }

                // Client -> Server
                if (FD_ISSET(client_fd, &fds)) {
                    ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
                    if (n <= 0) break;
                    send(server_fd, buf, n, 0);
                }

                // Server -> Client
                if (FD_ISSET(server_fd, &fds)) {
                    ssize_t n = recv(server_fd, buf, sizeof(buf), 0);
                    if (n <= 0) break;
                    send(client_fd, buf, n, 0);
                }
            }

            close(server_fd);
            return;
        }

        // -----------------------------------------------------------
        // Step 4: Connect to remote HTTP server
        // -----------------------------------------------------------
        int server_fd = connectToHost(host, port);
        if (server_fd < 0) {
            std::string resp = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
            send(client_fd, resp.c_str(), resp.size(), 0);
            return;
        }

        // -----------------------------------------------------------
        // Step 5: Forward the request to the server
        // -----------------------------------------------------------
        send(server_fd, request.c_str(), request.size(), 0);

        // -----------------------------------------------------------
        // Step 6: Read full HTTP response (headers + body)
        // -----------------------------------------------------------
        std::string response = readHttpResponse(server_fd);

        // If server closed connection unexpectedly, send what we have
        if (response.empty()) {
            close(server_fd);
            return;
        }

        // -----------------------------------------------------------
        // Step 7: Extract body for forbidden word scanning
        // -----------------------------------------------------------
        size_t header_end = response.find("\r\n\r\n");
        std::string headers = response.substr(0, header_end + 4);
        std::string body = response.substr(header_end + 4);

        // -----------------------------------------------------------
        // Step 8: Scan response body for forbidden words
        // -----------------------------------------------------------
        matches.clear();
        if (containsForbiddenWords(body, matches)) {
            std::string resp = make503Response(matches);
            send(client_fd, resp.c_str(), resp.size(), 0);
            close(server_fd);
            return;  // close client connection after error
        }

        // -----------------------------------------------------------
        // Step 9: If response is clean, forward it to the client
        // -----------------------------------------------------------
        send(client_fd, response.c_str(), response.size(), 0);

        // -----------------------------------------------------------
        // Step 10: Check for Connection: close
        // If either side wants to close, end the loop.
        // -----------------------------------------------------------
        std::string headers_lower = headers;
        std::transform(headers_lower.begin(), headers_lower.end(),
                       headers_lower.begin(), ::tolower);

        if (headers_lower.find("connection: close") != std::string::npos) {
            close(server_fd);
            return;
        }

        // Also stop if the client HTTP version is 1.0 (no keep-alive)
        if (request.rfind("HTTP/1.0", 0) != std::string::npos) {
            close(server_fd);
            return;
        }

        // Keep-alive: loop to read next request from client
        close(server_fd);
    }
}




// ====================================================================================================
// HTTP Request Reading
// ====================================================================================================
std::string HTTPProxyServer::readHttpRequest(int client_fd) {
    std::string request;
    char buf[8192];

    // ---------------------------------------------------------------
    // Step 1: Read until we have the complete HTTP header block
    // ---------------------------------------------------------------
    while (true) {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            // Client closed connection or error
            return "";
        }

        request.append(buf, n);

        // Check if we received the end of header section: "\r\n\r\n"
        if (request.find("\r\n\r\n") != std::string::npos)
            break;
    }

    // ---------------------------------------------------------------
    // Step 2: Extract headers and check for Content-Length
    // ---------------------------------------------------------------
    std::size_t header_end = request.find("\r\n\r\n");
    std::string headers = request.substr(0, header_end + 4);

    // Look for Content-Length header (case-insensitive not needed for this project)
    std::size_t cl_pos = headers.find("Content-Length:");
    if (cl_pos == std::string::npos) {
        // No body (GET, HEAD, some POSTs) → return headers only
        return request;
    }

    // Parse Content-Length number
    std::size_t value_start = headers.find_first_not_of(" \t", cl_pos + 15);
    std::size_t value_end   = headers.find("\r\n", value_start);
    std::string len_str     = headers.substr(value_start, value_end - value_start);

    size_t content_length = static_cast<size_t>(std::stoul(len_str));

    // ---------------------------------------------------------------
    // Step 3: If we already received some of the body in the header read,
    //         subtract what we already have.
    // ---------------------------------------------------------------
    size_t already_have = request.size() - (header_end + 4);
    size_t remaining = 0;

    if (already_have < content_length) {
        remaining = content_length - already_have;
    } else {
        // We already have full body (rare but possible if client sent everything in one packet)
        return request;
    }

    // ---------------------------------------------------------------
    // Step 4: Read remaining body bytes until we have the full request
    // ---------------------------------------------------------------
    std::string body_remainder = recvExact(client_fd, remaining);
    if (body_remainder.empty() && remaining > 0) {
        // Error or connection dropped
        return "";
    }

    request += body_remainder;

    return request;
}

bool HTTPProxyServer::containsForbiddenWords(const std::string& data,
                                             std::vector<std::string>& matches) const {
    matches.clear();

    // Lowercase copy of the input data
    std::string lowerData = data;
    std::transform(lowerData.begin(), lowerData.end(), lowerData.begin(), ::tolower);

    for (const std::string& word : forbidden_words) {
        // Lowercase copy of forbidden word
        std::string lowerWord = word;
        std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);

        // Case-insensitive substring search
        if (lowerData.find(lowerWord) != std::string::npos) {
            matches.push_back(word);  // return original casing for reporting
        }
    }

    return !matches.empty();
}



// ====================================================================================================
// Recv Helper:
// - Reads exactly 'n' bytes from fd unless connection closes.
// - Returns empty string on failure/EOF.
// ====================================================================================================
static std::string recvExact(int fd, size_t n) {
    std::string out;
    out.resize(n);

    size_t total = 0;
    while (total < n) {
        ssize_t bytes = recv(fd, &out[total], n - total, 0);

        if (bytes <= 0) {
            return {}; // Error or Connection Closed
        }
        total += bytes;
    }
    return out;
}



// ====================================================================================================
// HTTP Response Reading
// ====================================================================================================
std::string HTTPProxyServer::readHttpResponse(int server_fd) {
    // ---------------------------------------------------------------
    // Step 1: Read full headers
    // ---------------------------------------------------------------
    std::string headers = readHttpHeaders(server_fd);
    if (headers.empty()) {
        // Server closed connection or error
        return "";
    }

    // Prepare full HTTP response container
    std::string fullResponse = headers;

    // ---------------------------------------------------------------
    // Step 2: Decide body reading strategy
    // ---------------------------------------------------------------

    // --- Case A: Chunked Transfer-Encoding ---
    if (isChunked(headers)) {
        std::string body = readChunkedBody(server_fd);

        // If the server closed connection unexpectedly, bail out cleanly
        if (body.empty()) {
            std::cerr << "[HTTPProxyServer] Error: chunked body read failed.\n";
            return headers;  // return headers only; connection likely closing
        }

        fullResponse += body;
        return fullResponse;
    }

    // --- Case B: Content-Length: N ---
    size_t content_length = parseContentLength(headers);
    if (content_length > 0) {
        std::string body = readFixedLengthBody(server_fd, content_length);

        if (body.size() != content_length) {
            std::cerr << "[HTTPProxyServer] Warning: incomplete body read.\n";
        }

        fullResponse += body;
        return fullResponse;
    }

    // --- Case C: HTTP/1.0 EOF-terminated body ---
    // If there is no Content-Length and no chunked encoding,
    // many HTTP/1.0 servers simply close the socket to signal end-of-body.
    {
        std::string body;
        char buf[4096];

        while (true) {
            ssize_t n = recv(server_fd, buf, sizeof(buf), 0);
            if (n <= 0) {
                break;  // server closed connection or error
            }
            body.append(buf, n);
        }

        fullResponse += body;
        return fullResponse;
    }
}

std::string HTTPProxyServer::readHttpHeaders(int fd) {
    std::string headers;
    char buf[2048];

    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            // Server closed connection or error
            return "";
        }

        // Append newly received bytes into the header buffer
        headers.append(buf, n);

        // Check if we now have the full header block
        // HTTP headers end with CRLF CRLF ("\r\n\r\n")
        if (headers.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    return headers;
}


std::string HTTPProxyServer::readFixedLengthBody(int fd, size_t length) {
    std::string body;
    body.reserve(length);     // reduce reallocations
    char buf[4096];

    size_t total = 0;

    while (total < length) {
        // Determine how much to read this iteration
        size_t to_read = std::min(sizeof(buf), length - total);

        ssize_t n = recv(fd, buf, to_read, 0);
        if (n <= 0) {
            // Server closed connection early or error occurred
            std::cerr << "[HTTPProxyServer] readFixedLengthBody(): premature EOF ("
                      << total << "/" << length << " bytes)\n";
            break;
        }

        body.append(buf, n);
        total += n;
    }

    return body;
}

std::string HTTPProxyServer::readChunkedBody(int fd) {
    std::string body;              // final decoded body
    char buf[4096];

    while (true) {
        // -----------------------------------------------------------
        // Step 1: Read chunk-size line "<hex-size>\r\n"
        // -----------------------------------------------------------
        std::string chunk_header;
        while (true) {
            ssize_t n = recv(fd, buf, 1, 0);  // read 1 byte at a time
            if (n <= 0) {
                std::cerr << "[HTTPProxyServer] Chunked read: connection closed while reading chunk size.\n";
                return body;  // return what we got
            }
            chunk_header.push_back(buf[0]);

            // Stop when we hit CRLF
            if (chunk_header.size() >= 2 &&
                chunk_header[chunk_header.size() - 2] == '\r' &&
                chunk_header[chunk_header.size() - 1] == '\n') {
                break;
            }
        }

        // Remove trailing \r\n
        chunk_header.resize(chunk_header.size() - 2);

        // Some servers append chunk extensions; strip them:
        // Example: "1A;foo=bar"
        size_t semicolon = chunk_header.find(';');
        if (semicolon != std::string::npos) {
            chunk_header = chunk_header.substr(0, semicolon);
        }

        // -----------------------------------------------------------
        // Step 2: Parse hex chunk size
        // -----------------------------------------------------------
        size_t chunk_size = 0;
        try {
            chunk_size = std::stoul(chunk_header, nullptr, 16);
        } catch (...) {
            std::cerr << "[HTTPProxyServer] Invalid chunk size: '" << chunk_header << "'\n";
            return body; 
        }

        // -----------------------------------------------------------
        // Step 3: Check for last chunk (size = 0)
        // -----------------------------------------------------------
        if (chunk_size == 0) {
            // There should be a final CRLF after the 0-size chunk
            // Read exactly "\r\n"
            std::string ending;
            while (ending.size() < 2) {
                ssize_t n = recv(fd, buf, 1, 0);
                if (n <= 0) break;
                ending.push_back(buf[0]);
            }
            return body;  // done!
        }

        // -----------------------------------------------------------
        // Step 4: Read exactly 'chunk_size' bytes of chunk data
        // -----------------------------------------------------------
        size_t received = 0;
        while (received < chunk_size) {
            size_t to_read = std::min(sizeof(buf), chunk_size - received);
            ssize_t n = recv(fd, buf, to_read, 0);

            if (n <= 0) {
                std::cerr << "[HTTPProxyServer] Chunked body interrupted mid-chunk.\n";
                return body; 
            }

            body.append(buf, n);
            received += n;
        }

        // -----------------------------------------------------------
        // Step 5: Consume trailing CRLF after the chunk
        // -----------------------------------------------------------
        std::string trail;
        while (trail.size() < 2) {
            ssize_t n = recv(fd, buf, 1, 0);
            if (n <= 0) break;
            trail.push_back(buf[0]);
        }
    }

    // Should never reach here
    return body;
}



// ====================================================================================================
// Header Parsing Helpers
// ====================================================================================================
bool HTTPProxyServer::isChunked(const std::string& headers) const {
    // Make a lowercase copy for case-insensitive matching
    std::string lower = headers;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Look for "transfer-encoding: chunked"
    return lower.find("transfer-encoding: chunked") != std::string::npos;
}

size_t HTTPProxyServer::parseContentLength(const std::string& headers) const {
    // Lowercase copy for case-insensitive matching
    std::string lower = headers;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    const std::string key = "content-length:";
    size_t pos = lower.find(key);
    if (pos == std::string::npos) {
        return 0;  // No Content-Length header
    }

    // Position after "content-length:"
    pos += key.length();

    // Skip spaces/tabs
    while (pos < lower.size() && (lower[pos] == ' ' || lower[pos] == '\t')) {
        pos++;
    }

    // Extract the number up to CRLF
    size_t end = lower.find("\r\n", pos);
    if (end == std::string::npos) {
        return 0;  // malformed header
    }

    std::string value = lower.substr(pos, end - pos);

    try {
        return static_cast<size_t>(std::stoul(value));
    } catch (...) {
        // Malformed or too large number
        return 0;
    }
}



// ====================================================================================================
// Error Responses
// ====================================================================================================
std::string HTTPProxyServer::make403Response(const std::vector<std::string>& matches) const {
    // Build HTML Body Describing the Error and Listing Matched Forbidden Words
    std::ostringstream html;
    html << "<html><body>\n"
         << "<h2>403 Forbidden</h2>\n"
         << "<p>Your request contained forbidden content.</p>\n"
         << "<p>Blocked terms:</p>\n<ul>\n";

    // Add Each Forbidden Word as a List Item
    for (const auto& word : matches) {
        html << "  <li>" << word << "</li>\n";
    }

    // Close Prior Tags
    html << "</ul>\n"
         << "</body></html>";

    // Convert Body to String for Content-Length Computation
    std::string body = html.str();

    // Build Full HTTP Response with Proper Headers + Body
    std::ostringstream response;
    response << "HTTP/1.1 403 Forbidden\r\n"
             << "Content-Type: text/html; charset=UTF-8\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;

    return response.str();
}

std::string HTTPProxyServer::make503Response(const std::vector<std::string>& matches) const {
    // Build HTML Body Describing the Error and Listing Matched Forbidden Words
    std::ostringstream html;
    html << "<html><body>\n"
         << "<h2>503 Service Unavailable</h2>\n"
         << "<p>The server response contained forbidden content.</p>\n"
         << "<p>Blocked terms:</p>\n<ul>\n";

    // Add Each Forbidden Word as a List Item
    for (const auto& word : matches) {
        html << "  <li>" << word << "</li>\n";
    }

    // Close Prior Tags
    html << "</ul>\n"
         << "</body></html>";

    // Convert Body to String for Content-Length Computation
    std::string body = html.str();

    // Build Full HTTP Response with Proper Headers + Body
    std::ostringstream response;
    response << "HTTP/1.1 503 Service Unavailable\r\n"
             << "Content-Type: text/html; charset=UTF-8\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;

    return response.str();
}



// ====================================================================================================
// Networking Helpers
// ====================================================================================================
int HTTPProxyServer::connectToHost(const std::string& host, int port) {
    struct addrinfo hints {};
    hints.ai_family = AF_INET;        // IPv4 only
    hints.ai_socktype = SOCK_STREAM;  // TCP

    struct addrinfo* res = nullptr;

    std::string port_str = std::to_string(port);

    int err = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (err != 0) {
        std::cerr << "[HTTPProxyServer] DNS resolution failed for " << host
                  << ": " << gai_strerror(err) << "\n";
        return -1;
    }

    // Create socket
    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd < 0) {
        std::cerr << "[HTTPProxyServer] Failed to create socket for " << host << "\n";
        freeaddrinfo(res);
        return -1;
    }

    // Connect to server
    if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "[HTTPProxyServer] Failed to connect to "
                  << host << ":" << port
                  << " (" << strerror(errno) << ")\n";

        close(sock_fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock_fd;
}

bool HTTPProxyServer::parseHttpDestination(const std::string& request,
                                           std::string& out_host,
                                           int& out_port) {
    out_port = 80;  // default HTTP port

    std::istringstream stream(request);
    std::string method, url, version;

    // -----------------------------------------------------------
    // Parse initial request line
    // Example:
    //   GET /path HTTP/1.1
    //   CONNECT example.com:443 HTTP/1.1
    // -----------------------------------------------------------
    stream >> method >> url >> version;

    // -----------------------------------------------------------
    // Case 1: HTTPS Tunnel CONNECT request
    // -----------------------------------------------------------
    if (method == "CONNECT") {
        std::size_t colon = url.find(':');
        if (colon == std::string::npos) {
            return false;  // malformed CONNECT host:port
        }

        out_host = url.substr(0, colon);
        out_port = std::stoi(url.substr(colon + 1));
        return true;
    }

    // -----------------------------------------------------------
    // Case 2: Standard HTTP request (GET, POST, etc.)
    // We must find the Host header (case-insensitive).
    // -----------------------------------------------------------
    std::string lowerReq = request;
    std::transform(lowerReq.begin(), lowerReq.end(),
                   lowerReq.begin(), ::tolower);

    const std::string key = "host:";
    size_t host_pos = lowerReq.find(key);
    if (host_pos == std::string::npos) {
        return false;  // No Host header found
    }

    // -----------------------------------------------------------
    // Extract the host header *from the original request*,
    // because lowercase copy destroys the real casing.
    // -----------------------------------------------------------
    size_t start = request.find_first_not_of(" \t", host_pos + key.length());
    if (start == std::string::npos) {
        return false;
    }

    size_t end = request.find("\r\n", start);
    if (end == std::string::npos) {
        return false;
    }

    std::string host_line = request.substr(start, end - start);

    // -----------------------------------------------------------
    // Check if host:port format
    // -----------------------------------------------------------
    size_t colon = host_line.find(':');
    if (colon != std::string::npos) {
        out_host = host_line.substr(0, colon);
        out_port = std::stoi(host_line.substr(colon + 1));
    } else {
        out_host = host_line;
    }

    return true;
}



// ====================================================================================================
// Forbidden Word Loading
// ====================================================================================================
bool HTTPProxyServer::loadForbiddenWords() {
    forbidden_words.clear();

    // Load Forbidden Word File
    std::ifstream file("forbidden.txt");
    if (!file.is_open()) {
        std::cerr << "[HTTPProxyServer] Failed to open forbidden.txt\n";
        return false;
    }

    // Read Forbidden Words from File
    std::string line;
    while (std::getline(file, line)) {
        // Trim Whitespace from Both Ends
        auto start = line.find_first_not_of(" \t\r\n");
        auto end   = line.find_last_not_of(" \t\r\n");

        // Skip Empty Lines
        if (start == std::string::npos) continue;

        // Extract Word
        std::string word = line.substr(start, end - start + 1);

        // Skip Comment Lines (Beginning with '#')
        if (!word.empty() && word[0] == '#') continue;

        // Add Forbidden Word to List
        if (!word.empty()) {
            forbidden_words.push_back(word);
        }
    }

    // Log Forbidden Word Count to Console
    std::cout << "[HTTPProxyServer] Loaded " 
              << forbidden_words.size() 
              << " forbidden words.\n";

    return true;
}
