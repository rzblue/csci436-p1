#ifndef HTTP_PROXY_SERVER_HPP
#define HTTP_PROXY_SERVER_HPP

#include "BaseServer.hpp"
#include <string>
#include <vector>

class HTTPProxyServer : public BaseServer {
public:
    using BaseServer::BaseServer;   // inherit constructor

protected:
    void handleRequest(int client_fd) override;

private:
    // === Request Handling ===
    std::string readHttpRequest(int client_fd);
    bool containsForbiddenWords(const std::string& data, 
                                std::vector<std::string>& matches) const;


    // === Response Handling ===
    std::string readHttpResponse(int server_fd);
    std::string readHttpHeaders(int fd);
    std::string readFixedLengthBody(int fd, size_t length);
    std::string readChunkedBody(int fd);

    bool isChunked(const std::string& headers) const;
    size_t parseContentLength(const std::string& headers) const;

    // === Error Responses ===
    std::string make403Response(const std::vector<std::string>& matches) const;
    std::string make503Response(const std::vector<std::string>& matches) const;

    // === Networking ===
    int connectToHost(const std::string& host, int port);
    bool parseHttpDestination(const std::string& request,
                              std::string& out_host,
                              int& out_port);

    // === Forbidden List ===
    bool loadForbiddenWords();
    std::vector<std::string> forbidden_words;
};

#endif // HTTP_PROXY_SERVER_HPP
