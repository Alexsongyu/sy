#ifndef __SY_NS_NAME_SERVER_MODULE_H__
#define __SY_NS_NAME_SERVER_MODULE_H__

#include "sy/module.h"
#include "ns_protocol.h"

namespace sy {
namespace ns {

class NameServerModule;
class NSClientInfo {
friend class NameServerModule;
public:
    typedef std::shared_ptr<NSClientInfo> ptr;
private:
    NSNode::ptr m_node;
    std::map<std::string, std::set<uint32_t> > m_domain2cmds;
};

class NameServerModule : public RockModule {
public:
    typedef std::shared_ptr<NameServerModule> ptr;
    NameServerModule();

    virtual bool handleRockRequest(sy::RockRequest::ptr request
                        ,sy::RockResponse::ptr response
                        ,sy::RockStream::ptr stream) override;
    virtual bool handleRockNotify(sy::RockNotify::ptr notify
                        ,sy::RockStream::ptr stream) override;
    virtual bool onConnect(sy::Stream::ptr stream) override;
    virtual bool onDisconnect(sy::Stream::ptr stream) override;
    virtual std::string statusString() override;
private:
    bool handleRegister(sy::RockRequest::ptr request
                        ,sy::RockResponse::ptr response
                        ,sy::RockStream::ptr stream);
    bool handleQuery(sy::RockRequest::ptr request
                        ,sy::RockResponse::ptr response
                        ,sy::RockStream::ptr stream);
    bool handleTick(sy::RockRequest::ptr request
                        ,sy::RockResponse::ptr response
                        ,sy::RockStream::ptr stream);

private:
    NSClientInfo::ptr get(sy::RockStream::ptr rs);
    void set(sy::RockStream::ptr rs, NSClientInfo::ptr info);

    void setQueryDomain(sy::RockStream::ptr rs, const std::set<std::string>& ds);

    void doNotify(std::set<std::string>& domains, std::shared_ptr<NotifyMessage> nty);

    std::set<sy::RockStream::ptr> getStreams(const std::string& domain);
private:
    NSDomainSet::ptr m_domains;

    sy::RWMutex m_mutex;
    std::map<sy::RockStream::ptr, NSClientInfo::ptr> m_sessions;

    /// sessoin 关注的域名
    std::map<sy::RockStream::ptr, std::set<std::string> > m_queryDomains;
    /// 域名对应关注的session
    std::map<std::string, std::set<sy::RockStream::ptr> > m_domainToSessions;
};

}
}

#endif
