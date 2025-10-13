#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <string>

#include "FileClient.hpp"
#include "Protocol.hpp"


FileClient::FileClient(const std::string& server_ip, int server_port)
    : BaseClient(server_ip, server_port) {}

    
FileClient::FileClient(const std::string& dest_ip, int dest_port,
                       const std::string& proxy_ip, int proxy_port)
    : BaseClient(dest_ip, dest_port, proxy_ip, proxy_port) {}


void FileClient::makeRequest() {
    // Main Command-Handling Loop
    std::string input;
    while (true) {
        // Prompt User, Read Input
        std::cout << PRINT_PROMPT;
        std::getline(std::cin, input);

        // Exit Case
        if (input == "exit") break;

        // Clear terminal
        if (input == "clear") {
            std::cout << "\033c" << std::endl;
            continue;
        }

        // Parse Command and Filename
        std::istringstream iss(input);
        std::string command, filename;
        iss >> command >> filename;

        // Convert Command to Lower Case
        std::transform(command.begin(), command.end(), command.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        // Handle User Commands
        if (command == "identify") {
            identify();
        } else if (command == "put") {
            if (filename.empty()) {
                std::cout << "Error: Missing file name.\n";
            } else {
                putFile(filename);
            }
        } else if (command == "get") {
            if (filename.empty()) {
                std::cout << "Error: Missing file name.\n";
            } else {
                getFile(filename);
            }
        } else {
            std::cout << "Unknown command.\n";
        }
    }
}

void FileClient::identify() {
    // Construct Command Header
    std::vector<char> header(Protocol::COMMAND_HEADER_SIZE, 0);

    // Serialize Command Header
    header[0] = static_cast<char>(Protocol::CommandID::IDENTIFY);

    // Send Command Header
    send(socket_fd, header.data(), header.size(), 0);
    std::cout << "Sent IDENTIFY\n";
}

void FileClient::getFile(const std::string& file_name) {
    // Construct Absolute File Path
    std::filesystem::path local_path = std::filesystem::current_path() / file_name;

    // Construct and Serialize Command Header
    std::vector<char> header(Protocol::COMMAND_HEADER_SIZE + 2 + file_name.size());
    header[0] = static_cast<char>(Protocol::CommandID::GET_FILE);
    std::memset(&header[1], 0, 3);
    Protocol::write_uint16(&header[4], static_cast<uint16_t>(file_name.size()));
    std::memcpy(&header[6], file_name.data(), file_name.size());

    // Send Command Header
    send(socket_fd, header.data(), header.size(), 0);

    Protocol::ReplyStatus reply = receiveReply(); //Get a reply from the server with the status of our request
    if (reply == Protocol::ReplyStatus::INVALID) { //The requested file does not exist on server
        //std::cerr << "Error: File does not exist on server:" << file_name << "\n";
        std::cerr << PRINT_ERROR << "File does not exist on server:" << file_name << "\n";
        return;
    }
    if (reply != Protocol::ReplyStatus::ACK) { //Some other issue occured
        std::cerr << PRINT_ERROR <<  "Server rejected GET_FILE request\n";
        return;
    }

    // Receive Header Buffer
    char header_buf[4096];
    ssize_t bytes_received = recv(socket_fd, header_buf, sizeof(header_buf), 0);
    if (bytes_received <= 0) {
        std::cerr << PRINT_ERROR << "Failed to receive file header\n";
        return;
    }

    // Parse Header Buffer
    std::vector<char> buffer(header_buf, header_buf + bytes_received);
    Protocol::FileHeader file_header;
    size_t next_offset;
    if (!Protocol::FileHeader::parse(buffer, 0, file_header, next_offset)) {
        std::cerr << PRINT_ERROR << "Invalid file header received\n";
        return;
    }

    // Receive File Data
    std::vector<char> file_data;
    file_data.insert(file_data.end(), buffer.begin() + next_offset, buffer.end());
    while (file_data.size() < file_header.file_size) {
        char temp[4096];
        ssize_t n = recv(socket_fd, temp, sizeof(temp), 0);
        if (n <= 0) break;
        file_data.insert(file_data.end(), temp, temp + n);
    }

    // Validate File Size
    if (file_data.size() != file_header.file_size) {
        std::cerr << "File transfer incomplete. Expected "
                  << file_header.file_size << " bytes, received "
                  << file_data.size() << " bytes.\n";
        return;
    }

    // Write to Local File
    if (!writeFile(local_path, file_data)) {
        std::cerr << PRINT_ERROR << "Failed to save file to " << local_path << "\n";
        return;
    }
    std::cout << PRINT_SUCCESSES << "Downloaded file to " << local_path << "\n";
}

void FileClient::putFile(const std::string& file_name) {
    // Construct Absolute File Path
    std::filesystem::path local_path = std::filesystem::current_path() / file_name;

    // Read the Local File
    std::vector<char> file_data;
    if (!readFile(local_path, file_data)) {
        std::cerr << PRINT_ERROR << "Failed to read local file: " << local_path << "\n";
        return;
    }

    // Construct and Serialize the Payload
    std::vector<char> cmd_payload(Protocol::COMMAND_HEADER_SIZE + 2 + file_name.size());
    cmd_payload[0] = static_cast<char>(Protocol::CommandID::PUT_FILE);
    std::memset(&cmd_payload[1], 0, 3);
    Protocol::write_uint16(&cmd_payload[4], static_cast<uint16_t>(file_name.size()));
    std::memcpy(&cmd_payload[6], file_name.data(), file_name.size());

    // Send the Payload
    send(socket_fd, cmd_payload.data(), cmd_payload.size(), 0);

    // Receive Server Reply (ACK, NACK, ERROR)
    if (receiveReply() != Protocol::ReplyStatus::ACK) {
        std::cerr << PRINT_ERROR << "Server rejected PUT_FILE command\n";
        return;
    }

    // Construct and Serialize FileHeader
    Protocol::FileHeader header{0644, file_name, file_data.size()};
    std::vector<char> header_buffer(2 + 2 + header.path.size() + 8);
    Protocol::write_uint16(&header_buffer[0], header.permissions);
    Protocol::write_uint16(&header_buffer[2], header.path.size());
    std::memcpy(&header_buffer[4], header.path.data(), header.path.size());
    Protocol::write_uint64(&header_buffer[4 + header.path.size()], header.file_size);

    // Send FileHeader and File Data
    send(socket_fd, header_buffer.data(), header_buffer.size(), 0);
    send(socket_fd, file_data.data(), file_data.size(), 0);

    // Receive Final Server Reply
    if (receiveReply() == Protocol::ReplyStatus::ACK) {
        std::cout << PRINT_SUCCESSES << "Successfully uploaded file\n";
    } else {
        std::cerr << PRINT_ERROR << "Server failed to receive file\n";
    }
}

Protocol::ReplyStatus FileClient::receiveReply() {
    uint8_t reply;
    ssize_t n = recv(socket_fd, &reply, sizeof(reply), 0);
    if (n <= 0) {
        std::cerr << PRINT_ERROR << "Failed to receive reply\n";
        return Protocol::ReplyStatus::ERROR;
    }
    return static_cast<Protocol::ReplyStatus>(reply);
}

bool FileClient::readFile(const std::string& file_path, std::vector<char>& buffer) {
    // Create File Stream, Open File
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file) return false;

    // Check Exists
    std::streamsize size = file.tellg();
    if (size < 0) return false;

    // Read File Contents
    buffer.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    return file.read(buffer.data(), size).good();
}

bool FileClient::writeFile(const std::string& file_path, const std::vector<char>& buffer) {
    // Create File Stream, Open File
    std::ofstream file(file_path, std::ios::binary);
    if (!file) return false;

    // Write File Contents
    file.write(buffer.data(), buffer.size());
    return file.good();
}
