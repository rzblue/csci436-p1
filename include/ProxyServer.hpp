#ifndef PROXY_SERVER_HPP
#define PROXY_SERVER_HPP

#include "BaseServer.hpp"


class ProxyServer : public BaseServer {
public:
    using BaseServer::BaseServer; // Inherit constructor

protected:
    void handleRequest(int client_fd) override;
};

#endif // PROXY_SERVER_HPP