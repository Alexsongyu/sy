#include "sy/sy.h"
#include "sy/rock/rock_stream.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

sy::RockConnection::ptr conn(new sy::RockConnection);
void run() {
    conn->setAutoConnect(true);
    sy::Address::ptr addr = sy::Address::LookupAny("127.0.0.1:8061");
    if(!conn->connect(addr)) {
        SY_LOG_INFO(g_logger) << "connect " << *addr << " false";
    }
    conn->start();

    sy::IOManager::GetThis()->addTimer(1000, [](){
        sy::RockRequest::ptr req(new sy::RockRequest);
        static uint32_t s_sn = 0;
        req->setSn(++s_sn);
        req->setCmd(100);
        req->setBody("hello world sn=" + std::to_string(s_sn));

        auto rsp = conn->request(req, 300);
        if(rsp->response) {
            SY_LOG_INFO(g_logger) << rsp->response->toString();
        } else {
            SY_LOG_INFO(g_logger) << "error result=" << rsp->result;
        }
    }, true);
}

int main(int argc, char** argv) {
    sy::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
