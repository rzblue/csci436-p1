#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

#include "Protocol.hpp"


class Client {
public:
    Client(const std::string& server_ip, int server_port);
    ~Client();

    void start();

private:
    std::string server_ip;
    int server_port;
    int socket_fd;

    void connectToServer();

    void identify();
    void getFile(const std::string& remote_path, const std::string& local_path);
    void putFile(const std::string& local_path, const std::string& remote_path);

    void sendCommand(Protocol::CommandID command_id, const std::vector<char>& data);
    Protocol::ReplyStatus receiveReply();

    bool readFile(const std::string& path, std::vector<char>& buffer);
    bool writeFile(const std::string& path, const std::vector<char>& buffer);
};

#endif // CLIENT_HPP
