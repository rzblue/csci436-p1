#include <iostream>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>

#include "FileServer.hpp"
#include "Protocol.hpp"

FileServer::FileServer(int port)
    : BaseServer(port) {}

    
void FileServer::handleIdentify(const std::vector<char>& data) {
    std::string id(data.begin(), data.end());
    std::cout << "IDENTIFY command with id: " << id << ".\n";
}


void FileServer::handleGetFile(int client_fd, const std::string& file_name) {
    std::vector<char> file_data;

    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path local_path = current_path / file_name;

    if (!readFile(local_path.string(), file_data)) {
        std::cerr << "GET_FILE: Failed to read file: " << file_name << "\n";
        Protocol::sendReply(client_fd, Protocol::ReplyStatus::NACK);
        return;
    }

    // Build and send the FileHeader
    Protocol::FileHeader header;
    header.permissions = 0644; // TODO: optionally fetch real file mode
    header.path = local_path;
    header.file_size = file_data.size();

    // Serialize FileHeader
    std::vector<char> header_buffer;
    header_buffer.resize(2 + 2 + header.path.size() + 8);

    Protocol::write_uint16(&header_buffer[0], header.permissions);
    Protocol::write_uint16(&header_buffer[2], header.path.size());
    std::memcpy(&header_buffer[4], header.path.data(), header.path.size());
    Protocol::write_uint64(&header_buffer[4 + header.path.size()], header.file_size);

    // Send header
    if (send(client_fd, header_buffer.data(), header_buffer.size(), 0) != static_cast<ssize_t>(header_buffer.size())) {
        std::cerr << "GET_FILE: Failed to send file header\n";
        return;
    }

    // Send file data
    size_t total_sent = 0;
    while (total_sent < file_data.size()) {
        ssize_t sent = send(client_fd, file_data.data() + total_sent,
                            file_data.size() - total_sent, 0);
        if (sent <= 0) {
            std::cerr << "GET_FILE: Failed to send file contents\n";
            return;
        }
        total_sent += sent;
    }

    std::cout << "GET_FILE: Sent file '" << file_name << "' (" << total_sent << " bytes)\n";
}


void FileServer::handlePutFile(int client_fd, const std::vector<char>& file_data,
                               uint16_t permissions, const std::string& file_name) {
    std::filesystem::path local_path = std::filesystem::current_path() / file_name;

    std::cout << "PUT_FILE command for path: " << file_name
              << " with permissions: " << std::oct << permissions
              << " and file size: " << file_data.size() << std::dec << ".\n";
    
    // Write file data to disk
    if (!writeFile(local_path.string(), file_data)) {
        std::cerr << "PUT_FILE: Failed to write file to " << local_path << "\n";
        Protocol::sendReply(client_fd, Protocol::ReplyStatus::NACK);
        return;
    }

    // Set file permission on Linux; ignore on Windows
    #ifndef _WIN32
        if (chmod(local_path.c_str(), permissions) != 0) {
            std::cerr << "PUT_FILE: Failed to set permissions on " << file_name << "\n";
            Protocol::sendReply(client_fd, Protocol::ReplyStatus::NACK);
            return;
        }
    #else
        std::cout << "PUT_FILE: Skipping chmod on Windows.\n";
    #endif

    // Send ACK to client on success
    Protocol::sendReply(client_fd, Protocol::ReplyStatus::ACK);
    std::cout << "PUT_FILE: Successfully saved file '" << file_name << "'\n";
}


void FileServer::handleRequest(int client_fd) {
    constexpr size_t BUFFER_SIZE = 4096;
    std::vector<char> buffer;
    char temp_buffer[BUFFER_SIZE];

    ssize_t bytes_received;
    bool command_acknowledged = false;

    while ((bytes_received = recv(client_fd, temp_buffer, BUFFER_SIZE, 0)) > 0) {
        buffer.insert(buffer.end(), temp_buffer, temp_buffer + bytes_received);

        while (true) {
            if (buffer.size() < Protocol::COMMAND_HEADER_SIZE)
                break;

            Protocol::CommandHeader header;
            if (!Protocol::CommandHeader::parse(buffer, header))
                break;

            size_t cursor = Protocol::COMMAND_HEADER_SIZE;
            auto cmd = header.command_id;

            // Send ACK on Command Received
            if (!command_acknowledged) {
                std::cout << ">>ACK\n";
                Protocol::sendReply(client_fd, Protocol::ReplyStatus::ACK);
                command_acknowledged = true;
            }

            if (cmd == Protocol::CommandID::IDENTIFY) {
                std::string client_id(buffer.begin() + cursor, buffer.end());
                std::cout << "IDENTIFY command: client ID = " << client_id << "\n";
                buffer.clear();
                command_acknowledged = false;
            }

            else if (cmd == Protocol::CommandID::GET_FILE) {
                if (buffer.size() < cursor + 2) break;
                uint16_t path_len = Protocol::parse_uint16(&buffer[cursor]);
                cursor += 2;

                if (buffer.size() < cursor + path_len) break;
                std::string path_name(&buffer[cursor], path_len);
                cursor += path_len;

                handleGetFile(client_fd, path_name);
                buffer.erase(buffer.begin(), buffer.begin() + cursor);
                command_acknowledged = false;
            }

            else if (cmd == Protocol::CommandID::PUT_FILE) {
                if (buffer.size() < cursor + 2) break;
                uint16_t cmd_path_len = Protocol::parse_uint16(&buffer[cursor]);
                cursor += 2;

                if (buffer.size() < cursor + cmd_path_len) break;
                std::string cmd_path(&buffer[cursor], cmd_path_len);
                cursor += cmd_path_len;

                Protocol::FileHeader file_header;
                size_t next_offset;
                if (!Protocol::FileHeader::parse(buffer, cursor, file_header, next_offset))
                    break;

                cursor = next_offset;

                if (buffer.size() < cursor + file_header.file_size) break;

                std::vector<char> file_data(buffer.begin() + cursor, buffer.begin() + cursor + file_header.file_size);
                cursor += file_header.file_size;

                handlePutFile(client_fd, file_data, file_header.permissions, file_header.path);
                buffer.erase(buffer.begin(), buffer.begin() + cursor);
                command_acknowledged = false;
            }

            else if (cmd == Protocol::CommandID::ENUMERATE) {
                std::cout << "ENUMERATE command received (not implemented)\n";
                buffer.erase(buffer.begin(), buffer.begin() + Protocol::COMMAND_HEADER_SIZE);
                command_acknowledged = false;
            }

            else {
                std::cerr << "Unknown command ID: " << static_cast<int>(cmd) << "\n";
                buffer.clear();
                command_acknowledged = false;
            }

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


bool FileServer::readFile(const std::string& file_path, std::vector<char>& buffer) {
    // Create File Stream, Open File
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file) return false;

    // Get File Size
    std::streamsize size = file.tellg();
    if (size < 0) return false;

    // Read File Contents
    buffer.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    return file.read(buffer.data(), size).good();
}


bool FileServer::writeFile(const std::string& file_path, const std::vector<char>& buffer) {
    // Create File Stream, Open File
    std::ofstream file(file_path, std::ios::binary);
    if (!file) return false;

    // Write File Contents
    file.write(buffer.data(), buffer.size());
    return file.good();
}
