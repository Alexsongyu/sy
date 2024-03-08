#include "sy/sy.h"
#include "sy/ns/ns_protocol.h"
#include "sy/ns/ns_client.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

int type = 0;

void run() {
    g_logger->setLevel(sy::LogLevel::INFO);
    auto addr = sy::IPAddress::Create("127.0.0.1", 8072);
    //if(!conn->connect(addr)) {
    //    SY_LOG_ERROR(g_logger) << "connect to: " << *addr << " fail";
    //    return;
    //}
    if(type == 0) {
        for(int i = 0; i < 5000; ++i) {
            sy::RockConnection::ptr conn(new sy::RockConnection);
            conn->connect(addr);
            sy::IOManager::GetThis()->addTimer(3000, [conn, i](){
                    sy::RockRequest::ptr req(new sy::RockRequest);
                    req->setCmd((int)sy::ns::NSCommand::REGISTER);
                    auto rinfo = std::make_shared<sy::ns::RegisterRequest>();
                    auto info = rinfo->add_infos();
                    info->set_domain(std::to_string(rand() % 2) + "domain.com");
                    info->add_cmds(rand() % 2 + 100);
                    info->add_cmds(rand() % 2 + 200);
                    info->mutable_node()->set_ip("127.0.0.1");
                    info->mutable_node()->set_port(1000 + i);
                    info->mutable_node()->set_weight(100);
                    req->setAsPB(*rinfo);

                    auto rt = conn->request(req, 100);
                    SY_LOG_INFO(g_logger) << "[result="
                        << rt->result << " response="
                        << (rt->response ? rt->response->toString() : "null")
                        << "]";
            }, true);
            conn->start();
        }
    } else {
        for(int i = 0; i < 1000; ++i) {
            sy::ns::NSClient::ptr nsclient(new sy::ns::NSClient);
            nsclient->init();
            nsclient->addQueryDomain(std::to_string(i % 2) + "domain.com");
            nsclient->connect(addr);
            nsclient->start();
            SY_LOG_INFO(g_logger) << "NSClient start: i=" << i;

            if(i == 0) {
                //sy::IOManager::GetThis()->addTimer(1000, [nsclient](){
                //    auto domains = nsclient->getDomains();
                //    domains->dump(std::cout, "    ");
                //}, true);
            }
        }

        //conn->setConnectCb([](sy::AsyncSocketStream::ptr ss) {
        //    auto conn = std::dynamic_pointer_cast<sy::RockConnection>(ss);
        //    sy::RockRequest::ptr req(new sy::RockRequest);
        //    req->setCmd((int)sy::ns::NSCommand::QUERY);
        //    auto rinfo = std::make_shared<sy::ns::QueryRequest>();
        //    rinfo->add_domains("0domain.com");
        //    req->setAsPB(*rinfo);
        //    auto rt = conn->request(req, 1000);
        //    SY_LOG_INFO(g_logger) << "[result="
        //        << rt->result << " response="
        //        << (rt->response ? rt->response->toString() : "null")
        //        << "]";
        //    return true;
        //});

        //conn->setNotifyHandler([](sy::RockNotify::ptr nty,sy::RockStream::ptr stream){
        //        auto nm = nty->getAsPB<sy::ns::NotifyMessage>();
        //        if(!nm) {
        //            SY_LOG_ERROR(g_logger) << "invalid notify message";
        //            return true;
        //        }
        //        SY_LOG_INFO(g_logger) << sy::PBToJsonString(*nm);
        //        return true;
        //});
    }
}

int main(int argc, char** argv) {
    if(argc > 1) {
        type = 1;
    }
    sy::IOManager iom(5);
    iom.schedule(run);
    return 0;
}
