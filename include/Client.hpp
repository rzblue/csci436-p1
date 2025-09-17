#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <vector>

enum CommandType
{
  Put,
  Get,
  Unknown
};

class Client {
public:
    Client(int port, const char* remoteHostName);
    ~Client();

    void sendMessage();
    void handleGetFile();
    void handlePutFile();
    void recieveUserInput(std::string_view inputLine);

private:
    int clientSocket;
    int port;
    const char* remoteHostName;

    std::vector<std::string> format_input(std::string_view inputLine);
    CommandType getCommandType(std::string_view inputLine);
};

#endif // CLIENT_HPP