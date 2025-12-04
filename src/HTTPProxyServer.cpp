#include "HTTPProxyServer.hpp"
#include "HTTPRequestParser.hpp"
#include "HTTPResponseParser.hpp"
#include "ErrorResponseBuilder.hpp"
#include "HTTPSTunnel.hpp"
#include "NetworkUtils.hpp"

#include <unistd.h>
#include <iostream>

// ====================================================================================================
// Constructor
// ====================================================================================================
HTTPProxyServer::HTTPProxyServer(int port, const std::string& forbidden_file) 
    : BaseServer(port), filter(forbidden_file) {
    
    if (filter.isEmpty()) {
        std::cerr << "[HTTPProxyServer] Warning: No forbidden words loaded.\n";
    }
}

// ====================================================================================================
// Main Request Handler
// ====================================================================================================
void HTTPProxyServer::handleRequest(int client_fd) {
    while (true) {
        // -------------------------------------------------------
        // STEP 1: Read and parse client request
        // -------------------------------------------------------
        std::string request = HTTPRequestParser::readRequest(client_fd);
        if (request.empty()) {
            return;  // Client disconnected
        }

        std::cout << "[Proxy] Received " 
                  << HTTPRequestParser::getMethod(request) 
                  << " request\n";

        // -------------------------------------------------------
        // STEP 2: Check for forbidden words in request
        // -------------------------------------------------------
        std::vector<std::string> matches;
        if (filter.containsForbiddenContent(request, matches)) {
            std::cout << "[Proxy] Request blocked (forbidden content)\n";
            NetworkUtils::sendData(client_fd, ErrorResponseBuilder::build403Forbidden(matches));
            return;
        }

        // -------------------------------------------------------
        // STEP 3: Parse destination host and port
        // -------------------------------------------------------
        auto dest = HTTPRequestParser::parseDestination(request);
        if (!dest.valid) {
            std::cerr << "[Proxy] Could not parse destination\n";
            NetworkUtils::sendData(client_fd, ErrorResponseBuilder::build400BadRequest("Invalid destination"));
            return;
        }

        std::cout << "[Proxy] Destination: " << dest.host << ":" << dest.port << "\n";

        // -------------------------------------------------------
        // STEP 4: Handle CONNECT method (HTTPS tunnel)
        // -------------------------------------------------------
        if (HTTPRequestParser::isConnectRequest(request)) {
            if (!HTTPSTunnel::establish(client_fd, dest.host, dest.port)) {
                NetworkUtils::sendData(client_fd, ErrorResponseBuilder::build502BadGateway(
                    "Could not establish tunnel to " + dest.host));
            }
            return;  // Tunnel handles everything, then closes
        }

        // -------------------------------------------------------
        // STEP 5: Connect to destination server
        // -------------------------------------------------------
        int server_fd = NetworkUtils::connectToHost(dest.host, dest.port);
        if (server_fd < 0) {
            std::cerr << "[Proxy] Failed to connect to " << dest.host << "\n";
            NetworkUtils::sendData(client_fd, ErrorResponseBuilder::build502BadGateway(
                "Could not connect to " + dest.host));
            return;
        }

        // -------------------------------------------------------
        // STEP 6: Remove Accept-Encoding header to prevent compressed responses
        // -------------------------------------------------------
        std::string modified_request = HTTPRequestParser::removeHeader(request, "Accept-Encoding");

        // -------------------------------------------------------
        // STEP 7: Forward request to server
        // -------------------------------------------------------
        if (!NetworkUtils::sendData(server_fd, modified_request)) {
            std::cerr << "[Proxy] Failed to send request to server\n";
            close(server_fd);
            return;
        }

        // -------------------------------------------------------
        // STEP 8: Read and parse server response
        // -------------------------------------------------------
        auto response = HTTPResponseParser::readResponse(server_fd);
        
        if (!response.valid || response.full.empty()) {
            std::cerr << "[Proxy] Invalid or empty response from server\n";
            close(server_fd);
            return;
        }

        std::cout << "[Proxy] Response status: " << response.status_code << "\n";

        // -------------------------------------------------------
        // STEP 9: Check for forbidden words in response body
        // -------------------------------------------------------
        matches.clear();
        if (filter.containsForbiddenContent(response.body, matches)) {
            std::cout << "[Proxy] Response blocked (forbidden content)\n";
            NetworkUtils::sendData(client_fd, ErrorResponseBuilder::build503ServiceUnavailable(matches));
            close(server_fd);
            return;
        }

        // -------------------------------------------------------
        // STEP 10: Forward clean response to client
        // -------------------------------------------------------
        if (!NetworkUtils::sendData(client_fd, response.full)) {
            std::cerr << "[Proxy] Failed to send response to client\n";
            close(server_fd);
            return;
        }

        std::cout << "[Proxy] Response forwarded successfully\n";

        // -------------------------------------------------------
        // STEP 11: Determine if connection should persist
        // -------------------------------------------------------
        bool request_keep_alive = HTTPRequestParser::shouldKeepAlive(request);
        bool response_keep_alive = HTTPResponseParser::shouldKeepAlive(response.headers);

        close(server_fd);  // Always close server connection

        if (!request_keep_alive || !response_keep_alive) {
            std::cout << "[Proxy] Closing connection\n";
            return;
        }

        std::cout << "[Proxy] Keeping connection alive for next request\n";
        // Loop continues to handle next request from same client
    }
}