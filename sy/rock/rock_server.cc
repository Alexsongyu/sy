#include "rock_server.h"
#include "sy/log.h"
#include "sy/module.h"

namespace sy {

static sy::Logger::ptr g_logger = SY_LOG_NAME("system");

RockServer::RockServer(const std::string& type
                       ,sy::IOManager* worker
                       ,sy::IOManager* io_worker
                       ,sy::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker) {
    m_type = type;
}

void RockServer::handleClient(Socket::ptr client) {
    SY_LOG_DEBUG(g_logger) << "handleClient " << *client;
    sy::RockSession::ptr session(new sy::RockSession(client));
    session->setWorker(m_worker);
    ModuleMgr::GetInstance()->foreach(Module::ROCK,
            [session](Module::ptr m) {
        m->onConnect(session);
    });
    session->setDisconnectCb(
        [](AsyncSocketStream::ptr stream) {
             ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [stream](Module::ptr m) {
                m->onDisconnect(stream);
            });
        }
    );
    session->setRequestHandler(
        [](sy::RockRequest::ptr req
           ,sy::RockResponse::ptr rsp
           ,sy::RockStream::ptr conn)->bool {
            //SY_LOG_INFO(g_logger) << "handleReq " << req->toString()
            //                         << " body=" << req->getBody();
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, req, rsp, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                rt = m->handleRequest(req, rsp, conn);
            });
            return rt;
        }
    ); 
    session->setNotifyHandler(
        [](sy::RockNotify::ptr nty
           ,sy::RockStream::ptr conn)->bool {
            SY_LOG_INFO(g_logger) << "handleNty " << nty->toString()
                                     << " body=" << nty->getBody();
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, nty, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                rt = m->handleNotify(nty, conn);
            });
            return rt;
        }
    );
    session->start();
}

}
