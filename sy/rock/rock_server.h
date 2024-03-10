#ifndef __SY_ROCK_SERVER_H__
#define __SY_ROCK_SERVER_H__

#include "sy/rock/rock_stream.h"
#include "sy/tcp_server.h"

namespace sy {

class RockServer : public TcpServer {
public:
    typedef std::shared_ptr<RockServer> ptr;
    RockServer(const std::string& type = "rock"
               ,sy::IOManager* worker = sy::IOManager::GetThis()
               ,sy::IOManager* io_worker = sy::IOManager::GetThis()
               ,sy::IOManager* accept_worker = sy::IOManager::GetThis());

protected:
    virtual void handleClient(Socket::ptr client) override;
};

}

#endif
