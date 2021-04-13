#include "net/dispatcher.h"
#include "http/httpclient.h"
#include "http/httpserver.h"
#include "http/response.h"
#include "base/TimerMgr.h"
#include "thread/threadpool.h"

#include <vector>
using namespace std;

int main(int argc, char* argv[])
{
#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }
#endif // WIN32



    base::thread::ThreadPool::getInstance()->start(2);
    base::http::HttpClient::Create();
    base::http::HttpServer::Create();

    //Http get请求示例
    //（如果在不同的线程执行请求操作，需要在ExecuteAtNextLoop函数内，以保证代码在主循环内执行）
    base::Dispatcher::instance().ExecuteAtNextLoop([](){
            vector< pair< string, string > > formParams;
            formParams.emplace_back("name", "tt");
            formParams.emplace_back("format", "json");

            //http request
            base::http::HttpClient::instance()->GetAsync("http://www.baidu.com", formParams, [](base::http::HttpStatusCode code, const std::string& body, const std::string& request)
                {
                    std::cout << body << std::endl;
                }
            );
    });

    //Http Api服务示例
    base::http::HttpServer::instance()->AddHandler("/hello", [](const base::http::Request& req){
        base::http::Response resp;
        resp.AddHeader("Content-Type", "application/json; charset=utf-8");
        resp.SetContent("{\"hello\":\"world!\"}");
        req.SendResponse(resp);
        return true;
    });

    //开始侦听端口
    base::http::HttpServer::instance()->Start("0.0.0.0", 8001);

    //逻辑主循环
    base::Dispatcher::instance().Dispatch();

    base::thread::ThreadPool::getInstance()->stop();
    base::http::HttpClient::Destroy();
    base::http::HttpServer::Destroy();

#ifdef WIN32
    WSACleanup();
#endif // WIN32

	return 0;
}