#ifndef HTTP_PROXY_SERVER_HPP
#define HTTP_PROXY_SERVER_HPP

#include "BaseServer.hpp"
#include <string>
#include <netinet/in.h>


class HTTPProxyServer : public BaseServer {
public:
    using BaseServer::BaseServer; // Inherit Constructor

protected:
    void handleRequest(int client_fd) override;

private:
    bool parseHttpDestination(const std::string& request, std::string& out_host, int& out_port);
    int connectToHost(const std::string& host, int port);
};

#endif // HTTP_PROXY_SERVER_HPP
