#include "sy/http/http_server.h"
#include "sy/log.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

#define XX(...) #__VA_ARGS__


sy::IOManager::ptr worker;
void run() {
    g_logger->setLevel(sy::LogLevel::INFO);
    //sy::http::HttpServer::ptr server(new sy::http::HttpServer(true, worker.get(), sy::IOManager::GetThis()));
    sy::http::HttpServer::ptr server(new sy::http::HttpServer(true));
    sy::Address::ptr addr = sy::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/sy/xx", [](sy::http::HttpRequest::ptr req
                ,sy::http::HttpResponse::ptr rsp
                ,sy::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/sy/*", [](sy::http::HttpRequest::ptr req
                ,sy::http::HttpResponse::ptr rsp
                ,sy::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

    sd->addGlobServlet("/syx/*", [](sy::http::HttpRequest::ptr req
                ,sy::http::HttpResponse::ptr rsp
                ,sy::http::HttpSession::ptr session) {
            rsp->setBody(XX(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
));
            return 0;
    });

    server->start();
}

int main(int argc, char** argv) {
    sy::IOManager iom(1, true, "main");
    worker.reset(new sy::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
