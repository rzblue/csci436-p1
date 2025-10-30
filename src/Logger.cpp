#include "Logger.hpp"
#include <chrono>
#include <iostream>
#include <fmt/chrono.h>

// Create a new logger object for  the given client
Logger::Logger(std::string clientID){
    this->clientID = clientID;
}
// Destructor. TODO: Placeholder for now
Logger::~Logger(){
}

const std::string Logger::getTime(){
    return fmt::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::now());
}

void Logger::logRequest(std::string request){
    std::string log = getTime() + " [" + this->clientID + "]: Request: " + request;
    std::cout << log << std::endl; //TODO: For testing, remove for production
    //TODO: Write to log file
    //logToFile(log);
}

void Logger::logResponse(std::string response){
    std::string log = getTime() + " [" + this->clientID + "]: Response: " + response;
    std::cout << log << std::endl; //TODO: For testing, remove for production
    //TODO: Write to log file
    //logToFile(log);
}

void Logger::logToFile(std::string entry){
    //TODO: Write entry to log file(ex: log.txt). We can do just one log file for everthing,
    //or a log file per clientID (ex: log_client1.txt, log_client2.txt, etc)

}