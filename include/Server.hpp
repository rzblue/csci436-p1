#ifndef SERVER_HPP
#define SERVER_HPP

#include <string_view>
#include <vector>

class Server {
public:
    Server(int port);
    ~Server();

    void start();

private:
    int server_fd;
    int port;

    void handleIdentify(const std::vector<char>& data);
    void handleGetFile(int client_fd, const std::string& path);
    void handlePutFile(int client_fd, const std::vector<char>& file_data,
                       uint16_t permissions, const std::string& dest_path);

    void receive(int client_fd);

    bool readFile(std::string_view file_path, std::vector<char>& buffer);
    bool writeFile(std::string_view file_path, const std::vector<char>& buffer);
};

#endif // SERVER_HPP