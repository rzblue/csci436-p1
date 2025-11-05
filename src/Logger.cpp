#include "Logger.hpp"
#include <chrono>
#include <iostream>
#include <fmt/chrono.h>

#include <filesystem>   
#include <fstream>      
#include <mutex> 

#include <string>       

// Serialize concurrent file writes across all Logger instances
static std::mutex g_log_file_mutex;

// Sanitize a log line (trim CRLF, keep printable ASCII/whitespace, redact Basic auth, cap length)
static std::string sanitize_http_line(std::string s) {                    
    // Trim at first CRLF                                                 
    if (auto p = s.find("\r\n"); p != std::string::npos)                  
        s.erase(p);                                                       

    // Keep only printable ASCII and tab                                  
    std::string out;                                                      
    out.reserve(s.size());                                                
    for (unsigned char c : s) {                                           
        if ((c >= 32 && c <= 126) || c == '\t')                           
            out.push_back(static_cast<char>(c));                          
        // drop other control/binary                                      
    }                                                                     

    // Redact Basic auth if present                                        
    auto redact = [&](const char* key){                                  
        std::string k = key;                                             
        auto pos = out.find(k);                                           
        if (pos != std::string::npos) {                                   
            auto val_start = pos + k.size();                              
            // skip whitespace                                            
            while (val_start < out.size() &&                              
                   (out[val_start] == ' ' || out[val_start] == '\t'))     
                ++val_start;                                              
            // redact the rest of line                                     
            out.replace(val_start, out.size() - val_start, "[REDACTED]"); 
        }                                                                 
    };                                                                    
    redact("Authorization: Basic");                                       
    redact("Proxy-Authorization: Basic");                                 

    // Cap length                                                          
    constexpr size_t kMax = 512;                                          
    if (out.size() > kMax) {                                              
        out.resize(kMax);                                                 
        out += "…";                                                      
    }                                                                     
    return out;                                                           
}  


// Create a new logger object for the given client
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
    //Sanitize the request line before logging
    const std::string clean = sanitize_http_line(request); 

    std::string log = getTime() + " [" + this->clientID + "]: Request: " + clean; // <- Replace "clean" with "request" if sanitization isnt necessary
    std::cout << log << std::endl; //TODO: For testing, remove for production
    //TODO: Write to log file
    logToFile(log);
}

void Logger::logResponse(std::string response){
    // Sanitize the status line before logging
    const std::string clean = sanitize_http_line(response);


    std::string log = getTime() + " [" + this->clientID + "]: Response: " + clean;// <- Replace "clean" with "request" if sanitization isnt necessary
    std::cout << log << std::endl; //TODO: For testing, remove for production
    //TODO: Write to log file
    logToFile(log);
}

void Logger::logTunnelEstablished(std::string host, int port){
    logToFile(getTime() + " [" + this->clientID + "]: Tunnel established to" + host + ":" + std::to_string(port));
}
void Logger::logConnectionOpened(std::string host, int port){
    logToFile(getTime() + " [" + this->clientID + "]: Connection opened to " + host + ":" + std::to_string(port));
}
void Logger::logConnectionClosed(std::string host, int port){
    logToFile(getTime() + " [" + this->clientID + "]: Connection closed for " + host + ":" + std::to_string(port));
}
void Logger::logCustomMsg(std::string entry){
    logToFile(getTime() + " [" + this->clientID + "]: " + entry);
}


void Logger::logToFile(std::string entry){
    //TODO: Write entry to log file(ex: log.txt). We can do just one log file for everthing,
    //or a log file per clientID (ex: log_client1.txt, log_client2.txt, etc)
    
    // Note: Currently writing a log file per clientID.

    //const std::string logfil = "logs/client_" + this->clientID + ".log"; // Per-client log file
    const std::string logfile = "logs/log.txt"; // Log file for all clinets, uncomment if needed    std::lock_guard<std::mutex> lock(g_log_file_mutex); //Guard concurrent appends.

    std::ofstream out(logfile, std::ios::app); //Open in append mode
    if (!out){
        std::cerr << "[Logger] ERROR: cannot open " << logfile << "\n";
        return;
    }
    out << entry << '\n';
    //std::ofstream flushed on destruction; explicit flush not required.
}