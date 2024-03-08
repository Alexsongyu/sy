#ifndef __SY_HTTP_SERVLETS_CONFIG_SERVLET_H__
#define __SY_HTTP_SERVLETS_CONFIG_SERVLET_H__

#include "sy/http/servlet.h"

namespace sy {
namespace http {

class ConfigServlet : public Servlet {
public:
    ConfigServlet();
    virtual int32_t handle(sy::http::HttpRequest::ptr request
                   , sy::http::HttpResponse::ptr response
                   , sy::http::HttpSession::ptr session) override;
};

}
}

#endif
