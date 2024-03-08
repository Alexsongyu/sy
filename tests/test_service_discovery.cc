#include "sy/streams/service_discovery.h"
#include "sy/iomanager.h"
#include "sy/rock/rock_stream.h"
#include "sy/log.h"
#include "sy/worker.h"

sy::ZKServiceDiscovery::ptr zksd(new sy::ZKServiceDiscovery("127.0.0.1:21812"));
sy::RockSDLoadBalance::ptr rsdlb(new sy::RockSDLoadBalance(zksd));

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

std::atomic<uint32_t> s_id;
void on_timer() {
    g_logger->setLevel(sy::LogLevel::INFO);
    //SY_LOG_INFO(g_logger) << "on_timer";
    sy::RockRequest::ptr req(new sy::RockRequest);
    req->setSn(++s_id);
    req->setCmd(100);
    req->setBody("hello");

    auto rt = rsdlb->request("sy.top", "blog", req, 1000);
    if(!rt->response) {
        if(req->getSn() % 50 == 0) {
            SY_LOG_ERROR(g_logger) << "invalid response: " << rt->toString();
        }
    } else {
        if(req->getSn() % 1000 == 0) {
            SY_LOG_INFO(g_logger) << rt->toString();
        }
    }
}

void run() {
    zksd->setSelfInfo("127.0.0.1:2222");
    zksd->setSelfData("aaaa");

    std::unordered_map<std::string, std::unordered_map<std::string,std::string> > confs;
    confs["sy.top"]["blog"] = "fair";
    rsdlb->start(confs);
    //SY_LOG_INFO(g_logger) << "on_timer---";

    sy::IOManager::GetThis()->addTimer(1, on_timer, true);
}

int main(int argc, char** argv) {
    sy::WorkerMgr::GetInstance()->init({
        {"service_io", {
            {"thread_num", "1"}
        }}
    });
    sy::IOManager iom(1);
    iom.addTimer(1000, [](){
            std::cout << rsdlb->statusString() << std::endl;
    }, true);
    iom.schedule(run);
    return 0;
}
