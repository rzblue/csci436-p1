#ifndef BASE_CLIENT_HPP
#define BASE_CLIENT_HPP

#include <string>
#include <vector>


class BaseClient {
public:
    BaseClient(const std::string& ip, int port);  // Normal Direct Connection
    BaseClient(const std::string& dest_ip, int dest_port,
               const std::string& proxy_ip, int proxy_port); // Proxy-Enabled Connection
    ~BaseClient();

    void start();
    void disconnect();

protected:
    int socket_fd = -1;
    std::string server_ip;
    int server_port;

    bool use_proxy = false;
    std::string proxy_ip;
    int proxy_port = -1;

    virtual void makeRequest() = 0;
};

#endif // BASE_CLIENT_HPP