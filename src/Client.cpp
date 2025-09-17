#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "Client.hpp"

// Client Constructor
Client::Client(int serverPort, const char *remoteHostName)
{
    // Init
    this->port = serverPort;
    this->remoteHostName = remoteHostName;
    this->clientSocket = -1;
    struct hostent *resolvedHost;

    // Create TCP Socket
    this->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (this->clientSocket < 0)
    {
        std::cerr << "Error: Failed to create socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Get remote host name
    resolvedHost = gethostbyname(remoteHostName);
    if (!resolvedHost)
    {
        std::cerr << "Error: Could not find host " << remoteHostName << std::endl;
        exit(EXIT_FAILURE);
    }

    // Address
    sockaddr_in remoteAddress;
    std::memset(&remoteAddress, 0, sizeof(remoteAddress)); // Sets everything to 0
    remoteAddress.sin_family = AF_INET;
    std::memcpy(&remoteAddress.sin_addr, resolvedHost->h_addr, resolvedHost->h_length); // Copy the resolvede host address into remoteAddress
    remoteAddress.sin_port = htons(serverPort);

    // Open connection
    if (connect(this->clientSocket, (struct sockaddr *)&remoteAddress, sizeof(remoteAddress)) < 0)
    {
        std::cerr << "Error: Failed to connect to server" << std::endl;
    }
}

Client::~Client()
{
    // Close the Client Socket when the Object Goes out of Scope
    if (this->clientSocket != -1)
    {
        close(this->clientSocket);
        std::cout << "Client shut down." << std::endl;
    }
}
// TODO: This can probably be removed eventually, this is just for testing
void Client::sendMessage()
{
    char messageBuffer[256];
    int length;
    while (fgets(messageBuffer, sizeof(messageBuffer), stdin))
    {
        messageBuffer[255] = '\0';
        length = strlen(messageBuffer) + 1;
        send(clientSocket, messageBuffer, length, 0);
    }
}