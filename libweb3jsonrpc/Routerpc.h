#pragma once
#include "ModularServer.h"
#include "jsonrpccpp/server/abstractserverconnector.h"
#include <json/value.h>
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

    virtual void setRoutepath(const std::string& path, ModularServer<>* ms)
    {
        _route.push_back(std::pair<std::string, ModularServer<>*>(path, ms));
    }

    virtual void HandleRequest(
        const std::string& path, const std::string& request, std::string& retValue)
    {
        std::cout << "path " << path << std::endl;
        std::cout << "request " << request << std::endl;
        try
        {
            Json::CharReaderBuilder readerBuilder;
            Json::Value input;
            Json::Value output;
            Json::Value rque;

            std::unique_ptr<Json::CharReader> const jsonReader(readerBuilder.newCharReader());
            
            for(auto &itr : _route.rbegin(); itr != _route.rend();itr){
                if(itr.first == path){
                    itr.second.find_methos()
                }
            }
           
        }
        catch (const std::exception& e)
        {
            Json::Value v;
            v["error"] = "cant resolve json.s";
            retValue = v.toStyledString();
        }
    }

private:
    std::vector<std::pair<std::string, ModularServer<>*>> _route;
};


}  // namespace dev