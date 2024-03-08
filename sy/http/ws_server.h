#ifndef __SY_HTTP_WS_SERVER_H__
#define __SY_HTTP_WS_SERVER_H__

#include "sy/tcp_server.h"
#include "ws_session.h"
#include "ws_servlet.h"

namespace sy {
namespace http {

class WSServer : public TcpServer {
public:
    typedef std::shared_ptr<WSServer> ptr;

    WSServer(sy::IOManager* worker = sy::IOManager::GetThis()
             , sy::IOManager* io_worker = sy::IOManager::GetThis()
             , sy::IOManager* accept_worker = sy::IOManager::GetThis());

    WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch;}
    void setWSServletDispatch(WSServletDispatch::ptr v) { m_dispatch = v;}
protected:
    virtual void handleClient(Socket::ptr client) override;
protected:
    WSServletDispatch::ptr m_dispatch;
};

}
}

#endif
