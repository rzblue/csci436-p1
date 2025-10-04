#ifndef FILE_CLIENT_HPP
#define FILE_CLIENT_HPP

#include "Protocol.hpp"
#include "BaseClient.hpp"

#include <string>
#include <vector>


class FileClient : public BaseClient {
public:
    FileClient(const std::string& server_ip, int server_port);
    ~FileClient() = default;

    void start();

private:
    void identify();
    void getFile(const std::string& file_name);
    void putFile(const std::string& file_name);

    void sendCommand(Protocol::CommandID command_id, const std::vector<char>& data);
    Protocol::ReplyStatus receiveReply();

    // File helpers
    bool readFile(const std::string& path, std::vector<char>& buffer);
    bool writeFile(const std::string& path, const std::vector<char>& buffer);
};

#endif // FILE_CLIENT_HPP
