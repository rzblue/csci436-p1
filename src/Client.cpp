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
void Client::recieveUserInput(std::string_view inputLine)
{
    std::vector<std::string> words;
    words = format_input(inputLine);
    // Switch for different commands
    switch (getCommandType(words[0]))
    {
    case CommandType::Put:
        Client::handlePutFile();
        break;
    case CommandType::Get:
        Client::handleGetFile();
        break;
    default:
        std::cout << "Unknown Command." << std::endl;
    }
}

// Get what type of command the user entered.
CommandType Client::getCommandType(std::string_view inputLine)
{
    if (inputLine == "get" || inputLine == "Get")
    {
        return CommandType::Get;
    }
    if (inputLine == "put" || inputLine == "Put")
    {
        return CommandType::Put;
    }
    return CommandType::Unknown;
}

// This method takes a string, splits into a string vector based on spaces (words). This is for user input.
std::vector<std::string> Client::format_input(std::string_view inputLine)
{
    std::vector<std::string> words;
    std::string currentWord;
    // Check entire line for spaces
    for (size_t i = 0; i < inputLine.length(); i++)
    {
        if (inputLine[i] == ' ')
        { // We hit a space, so input all previous characters into the vector
            words.push_back(currentWord);
            currentWord.clear();
        }
        else
        { // No space, so add current character to the word
            currentWord = currentWord + inputLine[i];
        }
    }
    // Put in the last word
    words.push_back(currentWord);
    return words;
}
// What actions the client should do when given a put command
void Client::handlePutFile()
{
    std::cout << "TODO: Insert Client Action for putting to server!" << std::endl;
}
// What actions the client should do when given a get command
void Client::handleGetFile()
{
    std::cout << "TODO: Insert Client Action for getting from server!" << std::endl;
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