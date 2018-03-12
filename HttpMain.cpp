#include "net/dispatcher.h"
#include "http/httpclient.h"
#include "http/httpserver.h"
#include "http/response.h"


class TestHttpEventHandler : public base::http::HttpClientEventHandler
{
public:

    virtual void OnHttpClose(){}
    virtual void OnHttpResponse(base::http::HttpStatusCode code, const std::string& body)
    {

    }
};

TestHttpEventHandler g_TestHttpEventHandler;

int main(int argc, char* argv[])
{
    base::http::HttpClient::Create();
    base::http::HttpServer::Create();

    base::Dispatcher::instance().ExecuteAtNextLoop([](){

        auto getasync = []()
        {
            vector< pair< string, string > > formParams;
            formParams.emplace_back("name", "tt");
            formParams.emplace_back("format", "json");

            base::http::HttpClient::instance()->GetAsync("http://example.com/test", formParams, &g_TestHttpEventHandler);
        };

        UnsafeTimerInst.AddStdFunctionTimeRepeat(1, getasync, 5, -1);
    });

    base::http::HttpServer::instance()->AddHandler("/hello", [](const base::http::Request& req){
        base::http::Response resp;
        resp.AddHeader("Content-Type", "application/json; charset=utf-8");
        resp.SetContent("{\"hello\":\"world!\"}");
        req.SendResponse(resp);
        return true;
    });

    base::http::HttpServer::instance()->Start("0.0.0.0", m_iLocalPort);

    base::Dispatcher::instance().Dispatch();

    base::http::HttpClient::Destroy();
    base::http::HttpServer::Destroy();

	return 0;
}