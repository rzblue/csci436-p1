#ifndef CLIENT_HPP
#define CLIENT_HPP

class Client {
public:
    Client(int port, const char* remoteHostName);
    ~Client();

    void sendMessage();
    void handleGetFile();
    void handlePutFile();

private:
    int clientSocket;
    int port;
    const char* remoteHostName;
};

#endif // CLIENT_HPP