

#include "SafeHttpServer.h"
#include "jsonrpccpp/server/abstractserverconnector.h"
#include <thread>


// class IClientConnectionHandler {
//     public:
//         virtual ~IClientConnectionHandler() {}

//         virtual void HandleRequest(const std::string& request, std::string& retValue) = 0;
// };

namespace dev
{
namespace rpc
{
class defualtHandleRequest : public jsonrpc::IClientConnectionHandler
{
public:
    virtual ~defualtHandleRequest() {}
    virtual void HandleRequest(const std::string& request, std::string& retValue) {
        retValue = "default";
    }
};


class v1HanleRequest : public jsonrpc::IClientConnectionHandler
{
public:
    virtual ~v1HanleRequest() {}
    virtual void HandleRequest(const std::string& request, std::string& retValue) {
        retValue = "default v1";
    }
};
}  // namespace rpc
}  // namespace dev

int main(int argc, char const* argv[])
{
    dev::rpc::defualtHandleRequest dhr;
    dev::rpc::v1HanleRequest handle;
    /* code */
    dev::SafeHttpServer ss("0.0.0.0", 8080);
    ss.SetUrlHandler("/", &dhr);
    ss.SetUrlHandler("/v1", &handle);
    ss.StartListening();

    while (true)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    return 0;
}
