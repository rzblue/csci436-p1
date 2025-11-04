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

private:
    static void* threadEntry(void* arg);
    void threadHandler(int client_fd);
};

#endif // BASE_SERVER_HPP