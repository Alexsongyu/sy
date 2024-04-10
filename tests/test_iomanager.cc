#include "sy/sy.h"
#include "sy/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

sy::Logger::ptr g_logger = SY_LOG_ROOT();

int sock = 0;

void test_fiber() {
    SY_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    //sleep(3);

    //close(sock);
    //sy::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        SY_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        sy::IOManager::GetThis()->addEvent(sock, sy::IOManager::READ, [](){
            SY_LOG_INFO(g_logger) << "read callback";
        });
        sy::IOManager::GetThis()->addEvent(sock, sy::IOManager::WRITE, [](){
            SY_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            sy::IOManager::GetThis()->cancelEvent(sock, sy::IOManager::READ);
            close(sock);
        });
    } else {
        SY_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }

}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    sy::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}

sy::Timer::ptr s_timer;
void test_timer() {
    sy::IOManager iom(2, true, "IOM2");
    s_timer = iom.addTimer(1000, []() {
        static int i = 0;
        SY_LOG_INFO(g_logger) << "hello timer i = " << i;
        if (++i == 5) {
            s_timer->reset(2000, true);
        }
        if (i == 10) {
            s_timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv) {
    g_logger->setLevel(sy::LogLevel::INFO);
    //test1();
    test_timer();
    return 0;
}
