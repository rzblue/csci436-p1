#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <algorithm>          
#include <chrono>  
#include <cerrno>  

#include "ProxyServer.hpp"
#include "Protocol.hpp"

#include "Logger.hpp"

void ProxyServer::handleRequest(int client_fd) {
    Logger logger(std::to_string(client_fd)); //Create logger up-front so we can record early errors before connect.

    // Read Proxy Header
    Protocol::ProxyHeader header{};
    ssize_t bytes_read = recv(client_fd, &header, sizeof(header), MSG_WAITALL);
    if (bytes_read != sizeof(header)) {
        std::cerr << "[ProxyServer] Invalid or incomplete proxy header\n";
        logger.logResponse(std::string("ERROR invalid proxy header: bytes_read=") + std::to_string(bytes_read));
        return;
    }

    // Extract Destination Info
    std::string dest_ip(inet_ntoa(header.dest_addr));
    int dest_port = ntohs(header.dest_port);
    std::cout << "[ProxyServer] Connecting to destination "
              << dest_ip << ":" << dest_port << "\n";

    // Create Socket to Destination
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[ProxyServer] Failed to create server socket\n";
        logger.logResponse(std::string("ERROR socket(): ") + std::strerror(errno)); 
        return;
    }

    // Set Up Destination Address Structure
    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    dest_addr.sin_addr = header.dest_addr;

    // Connect to Destination Server
    if (connect(server_fd, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        std::cerr << "[ProxyServer] Failed to connect to "
                  << dest_ip << ":" << dest_port << "\n";
        logger.logResponse(std::string("ERROR connect(") + dest_ip + ":" + std::to_string(dest_port) + "): " + std::strerror(errno));  
        close(server_fd);
        return;
    }
    std::cout << "[ProxyServer] Connected to destination.\n";

    //Create a per-connection logger using the client_fd as ID
    Logger logger(std::to_string(client_fd));
    bool logged_request_line = false; //log first request once
    bool logged_status_line  = false; //log first status once

    // --- Session bookkeeping -------------------------------------------------
    std::size_t bytes_up = 0;                         // client -> server
    std::size_t bytes_down = 0;                       // server -> client
    auto t_start = std::chrono::steady_clock::now();  // session start time

    // Log a connection-open event so each file shows the target right away
    logger.logRequest(std::string("CONNECT ") + dest_ip + ":" + std::to_string(dest_port));
    // ------------------------------------------------------------------------

    // Relay Loop
    fd_set fds;  // Declare File Descriptor Set
    char relay_buffer[4096];
    while (true) {
        FD_ZERO(&fds);                                      // Clear the File Descriptor Set
        FD_SET(client_fd, &fds);                            // Add Client Socket to Set
        FD_SET(server_fd, &fds);                            // Add Server Socket to Set
        int max_fd = std::max(client_fd, server_fd) + 1;    // Compute Max FD for select()

        // Wait for Activity on Either Socket
        // Block Until Activity is Detected
        // Break Loop on Error
        if (select(max_fd, &fds, nullptr, nullptr, nullptr) < 0) {
            logger.logResponse(std::string("ERROR select(): ") + std::strerror(errno));
            break;
        }

        // Client → Server
        // Check if Client Socket is Ready for Reading
        if (FD_ISSET(client_fd, &fds)) {
            // Read Data from Client and Forward to Server
            ssize_t n = recv(client_fd, relay_buffer, sizeof(relay_buffer), 0);
            std::cout << "[ProxyServer] Relayed " << n << " bytes from client to server.\n";
            if (n <= 0) {
                if (n < 0) {
                    logger.logResponse(std::string("ERROR recv(client): ") + std::strerror(errno));
                } else {
                    logger.logResponse("INFO client closed connection");
                }
                break;
            }
                

            //Log the first request line once
            if (!logged_request_line) {                                      
                std::string chunk(relay_buffer, relay_buffer + n);           
                size_t eol = chunk.find("\r\n");                             
                std::string first_line = (eol == std::string::npos)         
                    ? chunk : chunk.substr(0, eol);                          
                if (first_line.size() > 512) first_line.resize(512);         
                logger.logRequest(first_line);                               
                logged_request_line = true;                                  
            }

            ssize_t s = send(server_fd, relay_buffer, n, 0);
            if (s < 0) {
                logger.logResponse(std::string("ERROR send(server): ") + std::strerror(errno));
                break;
            }
            bytes_up += static_cast<std::size_t>(s);   // Count bytes up
        }

        // Server → Client
        // Check if Server Socket is Ready for Reading
        if (FD_ISSET(server_fd, &fds)) {
            // Read Data from Server and Forward to Client
            ssize_t n = recv(server_fd, relay_buffer, sizeof(relay_buffer), 0);
            std::cout << "[ProxyServer] Relayed " << n << " bytes from server to client.\n";
            if (n <= 0) {
                if (n < 0) {
                    logger.logResponse(std::string("ERROR recv(server): ") + std::strerror(errno)); 
                } else {
                    logger.logResponse("INFO server closed connection");
                }
                break;
            }

            //Log the first status line once
            if (!logged_status_line) {                                      
                std::string chunk(relay_buffer, relay_buffer + n);           
                size_t eol = chunk.find("\r\n");                          
                std::string first_line = (eol == std::string::npos)         
                    ? chunk : chunk.substr(0, eol);                         
                if (first_line.size() > 512) first_line.resize(512);        
                logger.logResponse(first_line);                              
                logged_status_line = true;                               
            } 

            ssize_t s = send(client_fd, relay_buffer, n, 0);
            if (s < 0) {
                logger.logResponse(std::string("ERROR send(client): ") + std::strerror(errno));
                break;
            }
            bytes_down += static_cast<std::size_t>(s); // Count bytes down
        }
    }

    // --- Session summary on close -------------------------------------------
    auto t_end = std::chrono::steady_clock::now();                         
    std::chrono::duration<double> dt = t_end - t_start;                    
    char summary[128];                                                     
    std::snprintf(summary, sizeof(summary),                                
                  "SUMMARY up=%zub down=%zub dur=%.2fs",                   
                  bytes_up, bytes_down, dt.count());                       
    logger.logResponse(summary);                                           
    // -----------------------------------------------------------------------

    close(server_fd);
    std::cout << "[ProxyServer] Connection closed.\n";
}
