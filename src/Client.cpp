#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>

#include "Client.hpp"
#include "Protocol.hpp"


Client::Client(const std::string& server_ip, int server_port)
    : server_ip(server_ip), server_port(server_port), socket_fd(-1) {}


Client::~Client() {
    if (socket_fd != -1) {
        close(socket_fd);
        std::cout << "Disconnected from server.\n";
    }
}

void Client::start() {
    connectToServer();

    // Main Command-Handling Loop
    std::string input;
    while (true) {
        // Prompt User, Read Input
        std::cout << "> ";
        std::getline(std::cin, input);
        
        // Exit Case
        if (input == "exit") break;

        // Parse Command and Filename
        std::istringstream iss(input);
        std::string command, filename;
        iss >> command;
        iss >> filename;
        
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


void Client::connectToServer() {
    // Create a TCP Socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in Structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    // Connect to the Server via the Socket
    if (connect(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Connected to server at " << server_ip << ":" << server_port << "\n";
}


void Client::identify() {
    // Construct Command Header
    std::vector<char> header;
    header.resize(Protocol::COMMAND_HEADER_SIZE);

    // Serialize Command Header
    header[0] = static_cast<char>(Protocol::CommandID::IDENTIFY);

    // Send Command Header
    send(socket_fd, header.data(), header.size(), 0);
    std::cout << "Sent IDENTIFY\n";
}


void Client::getFile(const std::string& file_name) {
    // Construct Absolute File Path
    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path local_path = current_path / file_name;

    // Construct Command Header
    std::vector<char> header;
    size_t path_len = file_name.size();
    header.resize(Protocol::COMMAND_HEADER_SIZE + 2 + path_len);

    // Serialize Command Header
    header[0] = static_cast<char>(Protocol::CommandID::GET_FILE);
    std::memset(&header[1], 0, 3);
    Protocol::write_uint16(&header[4], static_cast<uint16_t>(path_len));
    std::memcpy(&header[6], file_name.data(), path_len);

    // Send Command Header
    send(socket_fd, header.data(), header.size(), 0);

    // Receive Server Reply (ACK, NACK, ERROR)
    if (receiveReply() != Protocol::ReplyStatus::ACK) {
        std::cerr << "Server rejected GET_FILE request\n";
        return;
    }

    // Receive Header Buffer
    char header_buf[4096];
    ssize_t bytes_received = recv(socket_fd, header_buf, sizeof(header_buf), 0);
    if (bytes_received <= 0) {
        std::cerr << "Failed to receive file header\n";
        return;
    }

    // Parse Header Buffer
    std::vector<char> buffer(header_buf, header_buf + bytes_received);
    Protocol::FileHeader file_header;
    size_t next_offset;
    if (!Protocol::FileHeader::parse(buffer, 0, file_header, next_offset)) {
        std::cerr << "Invalid file header received\n";
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

    // Write File to Disk
    if (!writeFile(local_path, file_data)) {
        std::cerr << "Failed to save file to " << local_path << "\n";
        return;
    }
    std::cout << "Downloaded file to " << local_path << "\n";
}


void Client::putFile(const std::string& file_name) {
    // Construct Absolute File Path
    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path local_path = current_path / file_name;
    
    // Read the Local File
    std::vector<char> file_data;
    if (!readFile(local_path, file_data)) {
        std::cerr << "Failed to read local file: " << local_path << "\n";
        return;
    }

    // Construct the Payload
    std::vector<char> cmd_payload;
    cmd_payload.resize(Protocol::COMMAND_HEADER_SIZE + 2 + file_name.size());

    // Serialize the Payload
    cmd_payload[0] = static_cast<char>(Protocol::CommandID::PUT_FILE);
    std::memset(&cmd_payload[1], 0, 3);
    Protocol::write_uint16(&cmd_payload[4], static_cast<uint16_t>(file_name.size()));
    std::memcpy(&cmd_payload[6], file_name.data(), file_name.size());

    // Send the Payload
    send(socket_fd, cmd_payload.data(), cmd_payload.size(), 0);
    
    // Receive Server Reply (ACK, NACK, ERROR)
    if (receiveReply() != Protocol::ReplyStatus::ACK) {
        std::cerr << "Server rejected PUT_FILE command\n";
        return;
    }
    std::cout << "Reply Received\n";
    
    // Construct FileHeader
    Protocol::FileHeader header;
    header.permissions = 0644;
    header.path = file_name;
    header.file_size = file_data.size();

    // Serialize FileHeader
    std::vector<char> header_buffer;
    header_buffer.resize(2 + 2 + header.path.size() + 8);
    Protocol::write_uint16(&header_buffer[0], header.permissions);
    Protocol::write_uint16(&header_buffer[2], header.path.size());
    std::memcpy(&header_buffer[4], header.path.data(), header.path.size());
    Protocol::write_uint64(&header_buffer[4 + header.path.size()], header.file_size);

    // Send FileHeader
    send(socket_fd, header_buffer.data(), header_buffer.size(), 0);

    // Send File Data
    size_t total_sent = 0;
    while (total_sent < file_data.size()) {
        ssize_t sent = send(socket_fd, file_data.data() + total_sent,
                            file_data.size() - total_sent, 0);
        if (sent <= 0) {
            std::cerr << "Failed to send file data\n";
            return;
        }
        total_sent += sent;
    }

    // Receive Server Reply
    if (receiveReply() == Protocol::ReplyStatus::ACK) {
        std::cout << "Successfully uploaded file\n";
    } else {
        std::cerr << "Server failed to receive file\n";
    }
}


Protocol::ReplyStatus Client::receiveReply() {
    uint8_t reply;
    ssize_t n = recv(socket_fd, &reply, sizeof(reply), 0);
    if (n <= 0) {
        std::cerr << "Failed to receive reply\n";
        return Protocol::ReplyStatus::ERROR;
    }
    return static_cast<Protocol::ReplyStatus>(reply);
}


bool Client::readFile(const std::string& file_path, std::vector<char>& buffer) {
    // Create File Stream, Open File
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "readFile: Failed to open file '" << file_path << "'\n";
        return false;
    }

    // Check Exists
    std::streamsize size = file.tellg();
    if (size < 0) {
        return false;
    }

    // Read File Contents
    buffer.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "readFile: Failed to read file contents\n";
        return false;
    }

    return true;
}


bool Client::writeFile(const std::string& file_path, const std::vector<char>& buffer) {
    // Create File Stream, Open File
    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "writeFile: Failed to open file '" << file_path << "' for writing\n";
        return false;
    }

    // Write File Contents
    file.write(buffer.data(), buffer.size());
    if (!file.good()) {
        std::cerr << "writeFile: Write failed\n";
        return false;
    }

    return true;
}

