#include <iostream>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Protocol.hpp"
#include "Server.hpp"


Server::Server(int port)
: server_fd{-1},
  port{port} {
    // Create a TCP Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server_fd < 0) {
        std::cerr << "Error: Failed to create socket\n";
        exit(EXIT_FAILURE);
    }

    // Set Socket Options to Allow Reuse of Address
    int opt = 1;
    if (setsockopt(this->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: Failed to set socket options\n";
        close(this->server_fd);
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in Structure
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Listen on all interfaces
    server_addr.sin_port = htons(port);         // Port (network byte order)

    // Bind the Socket to the Port
    if (bind(this->server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Failed to bind socket to port\n";
        close(this->server_fd);
        exit(EXIT_FAILURE);
    }

    // Start Listening for Incoming Connections (backog size = 5)
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Error: Failed to listen on port " << port << "\n";
        close(this->server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server successfully initialized on port " << this->port << ".\n";
}


Server::~Server() {
    // Close the Server Socket when the Object Goes out of Scope
    if (this->server_fd != -1) {
        close(this->server_fd);
        std::cout << "Server shut down.\n";
    }
}


void Server::start() {
    std::cout << "Waiting for incoming connections...\n";

    while (true) {
        // Accept a New Connection
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(this->server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Error: Failed to accept client connection\n";
            continue;
        }
        std::cout << "Client connected\n";

        // Receive Data from the Client and Print It
        receive(client_fd);

        // Close the Client Socket
        close(client_fd);
    }
}


// Helper parsing functions assuming big-endian (network byte order)
uint16_t parse_uint16(const char* data) {
    return (static_cast<uint8_t>(data[0]) << 8) | static_cast<uint8_t>(data[1]);
}


uint64_t parse_uint64(const char* data) {
    uint64_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) | static_cast<uint8_t>(data[i]);
    }
    return result;
}


// Placeholder command handlers to implement
void handleIdentify(const std::vector<char>& data) {
    std::string id(data.begin(), data.end());
    std::cout << "IDENTIFY command with id: " << id << ".\n";
}


void handleGetFile(int client_fd, const std::string& path) {
    std::cout << "GET_FILE command for path: " << path << ".\n";
    // TODO: read file, send contents to client_fd
}


void handlePutFile(int client_fd, const std::vector<char>& file_data,
                   uint16_t permissions, const std::string& dest_path) {

    std::cout << "PUT_FILE command for path: " << dest_path
              << " with permissions: " << std::oct << permissions
              << " and file size: " << file_data.size() << std::dec << ".\n";
    // TODO: write file_data to disk at dest_path with permissions
    //       send ACK to client_fd?
}


void Server::receive(int client_fd) {
    constexpr size_t BUFFER_SIZE = 4096;
    std::vector<char> buffer;
    char temp_buffer[BUFFER_SIZE];

    ssize_t bytes_received;

    while ((bytes_received = recv(client_fd, temp_buffer, BUFFER_SIZE, 0)) > 0) {
        buffer.insert(buffer.end(), temp_buffer, temp_buffer + bytes_received);

        while (true) {
            if (buffer.size() < Protocol::COMMAND_HEADER_SIZE)
                break;

            Protocol::CommandHeader header;
            if (!Protocol::CommandHeader::parse(buffer, header))
                break;

            size_t cursor = Protocol::COMMAND_HEADER_SIZE;

            switch (header.command_id) {
                case Protocol::CommandID::IDENTIFY: {
                    // Implementation-defined: treat remaining data as identifier string
                    std::string client_id(buffer.begin() + cursor, buffer.end());
                    std::cout << "IDENTIFY command: client ID = " << client_id << "\n";

                    buffer.clear();
                    break;
                }

                case Protocol::CommandID::GET_FILE: {
                    if (buffer.size() < cursor + 2) break;
                    uint16_t path_len = Protocol::parse_uint16(&buffer[cursor]);
                    cursor += 2;

                    if (buffer.size() < cursor + path_len) break;
                    std::string path_name(&buffer[cursor], path_len);
                    cursor += path_len;

                    handleGetFile(client_fd, path_name);

                    buffer.erase(buffer.begin(), buffer.begin() + cursor);
                    break;
                }

                case Protocol::CommandID::PUT_FILE: {
                    // Extract PUT_FILE's initial path_len and path_name
                    if (buffer.size() < cursor + 2) break;
                    uint16_t cmd_path_len = Protocol::parse_uint16(&buffer[cursor]);
                    cursor += 2;

                    if (buffer.size() < cursor + cmd_path_len) break;
                    std::string cmd_path(&buffer[cursor], cmd_path_len);
                    cursor += cmd_path_len;

                    // Pparse the FileHeader
                    Protocol::FileHeader file_header;
                    size_t next_offset;
                    if (!Protocol::FileHeader::parse(buffer, cursor, file_header, next_offset))
                        break;

                    cursor = next_offset;

                    // Read the file data
                    if (buffer.size() < cursor + file_header.file_size) break;

                    std::vector<char> file_data(buffer.begin() + cursor, buffer.begin() + cursor + file_header.file_size);
                    cursor += file_header.file_size;

                    handlePutFile(client_fd, file_data, file_header.permissions, file_header.path);

                    buffer.erase(buffer.begin(), buffer.begin() + cursor);
                    break;
                }

                case Protocol::CommandID::ENUMERATE: {
                    std::cout << "ENUMERATE command received (not implemented)\n";
                    buffer.erase(buffer.begin(), buffer.begin() + Protocol::COMMAND_HEADER_SIZE);
                    break;
                }

                default: {
                    std::cerr << "Unknown command ID: " << static_cast<int>(header.command_id) << "\n";
                    buffer.clear();
                    break;
                }
            }

            // If buffer is empty or incomplete, stop parsing
            if (buffer.empty() || buffer.size() < Protocol::COMMAND_HEADER_SIZE)
                break;
        }
    }

    if (bytes_received < 0) {
        std::cerr << "Error: Failed to receive data from client\n";
    } else {
        std::cout << "Client disconnected\n";
    }
}


bool Server::readFile(std::string_view file_path, std::vector<char>& buffer) {
    std::ifstream file(std::string(file_path), std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    std::streamsize size = file.tellg();
    if (size < 0) {
        return false;
    }

    buffer.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(buffer.data(), size)) {
        return false;
    }

    return true;
}


bool Server::writeFile(std::string_view file_path, const std::vector<char>& buffer) {
    std::ofstream file(std::string(file_path), std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(buffer.data(), buffer.size());
    return file.good();
}