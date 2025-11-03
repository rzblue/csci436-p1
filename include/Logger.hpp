#include <string>
#ifndef LOGGER_HPP
#define LOGGER_HPP

class Logger {
    public:
        Logger(std::string clientID);
        ~Logger();
        void logRequest(std::string rquest); //Log the request and timestamp
        void logResponse(std::string response); //Log the response and timestamp
    private:
        std::string clientID;
        const std::string getTime();
        void logToFile(std::string entry);
};

#endif // LOGGER_HPP