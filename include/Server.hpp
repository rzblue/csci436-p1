#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

class Server {
public:
    Server(int port);
    ~Server();

    void start();

private:
    int server_fd;
    int port;
};

#endif // SERVER_HPP