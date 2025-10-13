#ifndef FILE_CLIENT_HPP
#define FILE_CLIENT_HPP

#include "Protocol.hpp"
#include "BaseClient.hpp"

#include <string>
#include <vector>

// ANSI Color Codes for Terminal Output
#define RESET "\033[0m"
#define RED "\033[31m"     /* Red */
#define GREEN "\033[32m"   /* Green */
#define MAGENTA "\033[35m" /* Magenta */
#define PRINT_ERROR RED << "[ERROR]" << RESET << " "
#define PRINT_SUCCESSES GREEN << "[SUCCESSES]" << RESET << " "
#define PRINT_PROMPT MAGENTA << ">" << RESET << " "

class FileClient : public BaseClient {
public:
    FileClient(const std::string& server_ip, int server_port);
    FileClient(const std::string& dest_ip, int dest_port,
               const std::string& proxy_ip, int proxy_port);
    ~FileClient() = default;

protected:
    void makeRequest() override;

private:
    void identify();
    void getFile(const std::string& file_name);
    void putFile(const std::string& file_name);

    void sendCommand(Protocol::CommandID command_id, const std::vector<char>& data);
    Protocol::ReplyStatus receiveReply();

    // File helpers
    bool readFile(const std::string& path, std::vector<char>& buffer);
    bool writeFile(const std::string& path, const std::vector<char>& buffer);
};

#endif // FILE_CLIENT_HPP
