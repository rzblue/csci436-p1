#ifndef BASE_CLIENT_HPP
#define BASE_CLIENT_HPP

#include <string>
#include <vector>


class BaseClient {
public:
    BaseClient(const std::string& ip, int port);
    ~BaseClient();

    void start();
    void disconnect();

protected:
    int socket_fd;
    std::string server_ip;
    int server_port;

    virtual void makeRequest() = 0;
};

#endif // BASE_CLIENT_HPP