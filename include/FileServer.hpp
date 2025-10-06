#ifndef FILE_SERVER_HPP
#define FILE_SERVER_HPP

#include "BaseServer.hpp"
#include "Protocol.hpp"

#include <string>
#include <vector>


class FileServer : public BaseServer {
public:
    FileServer(int port);
    ~FileServer() = default;

protected:
    void handleRequest(int client_fd) override;

private:
    bool parseCommand(int client_fd, Protocol::CommandHeader& command_header, std::vector<char>& buffer);
    void acknowledgeCommand(int client_fd);

    void handleIdentify(const std::vector<char>& data);
    void handleGetFile(int client_fd, const std::string& path);
    void handlePutFile(int client_fd, const std::vector<char>& file_data,
                       uint16_t permissions, const std::string& dest_path);

    bool readFile(const std::string& file_path, std::vector<char>& buffer);
    bool writeFile(const std::string& file_path, const std::vector<char>& buffer);
};

#endif // FILE_SERVER_HPP