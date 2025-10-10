#ifndef BASE_SERVER_HPP
#define BASE_SERVER_HPP


class BaseServer{
public:
    BaseServer(int port);
    ~BaseServer();

    bool start();

    int acceptConnection();

protected:
    int socket_fd;
    int server_port;

    virtual void handleRequest(int client_fd) = 0;
};

#endif // BASE_SERVER_HPP