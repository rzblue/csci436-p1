#include <string>
#ifndef LOGGER_HPP
#define LOGGER_HPP

class Logger {
    public:
        Logger(std::string clientID);
        ~Logger();
        void logRequest(std::string request); //Log the request and timestamp
        void logResponse(std::string response); //Log the response and timestamp
        void logTunnelEstablished(std::string host, int port); //Log tunnel establishment
        void logConnectionOpened(std::string host, int port); //Log connection opening
        void logConnectionClosed(std::string host, int port); //Log connection closure
        void logCustomMsg(std::string entry); //Log custom message
    private:
        std::string clientID;
        const std::string getTime();
        void logToFile(std::string entry);
};

#endif // LOGGER_HPP