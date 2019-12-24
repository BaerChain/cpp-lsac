#pragma once
#include "ModularServer.h"
#include "jsonrpccpp/server/abstractserverconnector.h"
#include <iostream>


namespace dev
{
class RouteRpcInterface
{
public:
    virtual ~RouteRpcInterface() {}

    virtual void setRoutepath(const std::string& path, ModularServer<>* ms) = 0;
    virtual void HandleRequest(
        const std::string& path, const std::string& request, std::string& retValue) = 0;
};


class RouteRpc : public RouteRpcInterface
{
public:
    virtual ~RouteRpc() {}

    virtual void setRoutepath(const std::string& path, ModularServer<>* ms) {}

    virtual void HandleRequest(
        const std::string& path, const std::string& request, std::string& retValue)
    {
        std::cout << "path " << path << std::endl;
        std::cout << "request " << request << std::endl;
        retValue = "trest";
    }
};


}  // namespace dev