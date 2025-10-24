#include "Logger.hpp"
#include <chrono>
#include <iostream>
#include <fmt/chrono.h>

#include <filesystem>   
#include <fstream>      
#include <mutex> 


// Serialize concurrent file writes across all Logger instances
static std::mutex g_log_file_mutex;

// Create a new logger object for  the given client
Logger::Logger(std::string clientID){
    this->clientID = clientID;

    // Ensure the logs directory exists (safe if it already exists)
    std::error_code ec;
    std::filesystem::create_directories("logs", ec);
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
    logToFile(log);
}

void Logger::logResponse(std::string response){
    std::string log = getTime() + " [" + this->clientID + "]: Response: " + response;
    std::cout << log << std::endl; //TODO: For testing, remove for production
    //TODO: Write to log file
    logToFile(log);
}

void Logger::logToFile(std::string entry){
    //TODO: Write entry to log file(ex: log.txt). We can do just one log file for everthing,
    //or a log file per clientID (ex: log_client1.txt, log_client2.txt, etc)

    const std::string logfil = "logs/client_" + this->clientID + ".log"; // Per-client log file

    std::lock_guard<std::mutex> lock(g_log_file_mutex); //Guard concurrent appends.

    std::ofstream out(logfile, std::ios::app); //Open in append mode
    if (!out){
        std::cerr << "[Logger] ERROR: cannot open " << logfile << "\n";
        return;
    }
    out << entry << '\n';
    //std::ofstream flushed on destruction; explicit flush not required.
    }
}