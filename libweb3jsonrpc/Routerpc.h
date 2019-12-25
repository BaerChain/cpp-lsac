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
        Json::Value input;
        Json::Value response;
        try
        {
            Json::CharReaderBuilder readerBuilder;

            Json::Value output;
            Json::Value rque;
            JSONCPP_STRING errs;
            std::unique_ptr<Json::CharReader> const jsonReader(readerBuilder.newCharReader());
            bool res = jsonReader->parse(
                request.c_str(), request.c_str() + request.length(), &input, &errs);
            if (res)
            {
                bool skip_version = false;
                for (auto itr = _route.rbegin(); itr != _route.rend(); itr++)
                {
                    if (itr->first == path || skip_version)
                    {
                        if (!itr->second->find_methods(input["method"].asString()))
                        {
                            skip_version = true;
                        }
                        else
                        {
                            jsonrpc::Procedure pro(input["method"].asString(),
                                jsonrpc::parameterDeclaration_t::PARAMS_BY_NAME, NULL);
                            itr->second->HandleMethodCall(pro, input["params"], output);

                            response["result"] = output;
                        }
                    }
                }
            }
            else
            {
                response["error"] = Json::Value("cant resolve json.");
            }
        }
        catch (const jsonrpc::JsonRpcException& ex)
        {
            response["error"] = ex.what();
        }
        catch (const std::exception& e)
        {
            response["error"] = e.what();
        }
        catch (...){
            response["error"] = "unknow can not find METHOD";
        }
        response["jsonrpc"] = Json::Value("2.0");
        response["id"] = input["id"];
        retValue = response.toStyledString();
    }

private:
    std::vector<std::pair<std::string, ModularServer<>*>> _route;
};


}  // namespace dev