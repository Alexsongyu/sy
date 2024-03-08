#include "sy/email/email.h"
#include "sy/email/smtp.h"

void test() {
    sy::EMail::ptr email = sy::EMail::Create(
            "user@163.com", "passwd",
            "hello world", "<B>hi xxx</B>hell world", {"564628276@qq.com"});
    sy::EMailEntity::ptr entity = sy::EMailEntity::CreateAttach("sy/sy.h");
    if(entity) {
        email->addEntity(entity);
    }

    entity = sy::EMailEntity::CreateAttach("sy/address.cc");
    if(entity) {
        email->addEntity(entity);
    }

    auto client = sy::SmtpClient::Create("smtp.163.com", 465, true);
    if(!client) {
        std::cout << "connect smtp.163.com:25 fail" << std::endl;
        return;
    }

    auto result = client->send(email, true);
    std::cout << "result=" << result->result << " msg=" << result->msg << std::endl;
    std::cout << client->getDebugInfo() << std::endl;
    //result = client->send(email, true);
    //std::cout << "result=" << result->result << " msg=" << result->msg << std::endl;
    //std::cout << client->getDebugInfo() << std::endl;
}

int main(int argc, char** argv) {
    sy::IOManager iom(1);
    iom.schedule(test);
    iom.stop();
    return 0;
}
