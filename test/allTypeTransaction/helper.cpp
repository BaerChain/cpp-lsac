//
// Created by friday on 22/05/2019.
//


#include "helper.hpp"

#include <jsonrpccpp/client/connectors/httpclient.h>
namespace test_tools{



    std::string post_message(std::string const &_rlpStr, std::string const &_ip_port) {
        std::string _result;
        jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(_ip_port);
        _httpClient.SetTimeout(1000 * 20L);
        _httpClient.SendRPCMessage(_rlpStr, _result);
        return _result;
    }



    Json::Value string_to_json(const std::string &source) {
        auto readerBuilder = Json::CharReaderBuilder();
        auto sreader = readerBuilder.newCharReader();
        Json::Value root;
        std::string errotStr;
        if (!sreader->parse(source.c_str(), source.c_str() + source.size(), &root, &errotStr)) {
            std::cout << "TODO throw exception.\n";
        }
        return root;
    }



    template<typename T>
    void to_variant(const Json::Value &value, T &t){

    }


    template<>
    void to_variant<request_rpc>(const Json::Value &value, request_rpc &t) {
        t.jsonrpc = value["jsonrpc"].asString();
        t.id = value["id"].asString();
        t.result = value["result"];
    }


    template <>
    void to_variant<account>(const Json::Value &value, account &t)
    {
        t.address = dev::Address(value["Address"].asString());
        t.BRC =  u256(fromBigEndian<u256>(fromHex(value["BRC"].asString())));
        t.FBRC =  u256(fromBigEndian<u256>(fromHex(value["FBRC"].asString())));
        t.FBalance =  u256(fromBigEndian<u256>(fromHex(value["FBalance"].asString())));
        t.arrears =  u256(fromBigEndian<u256>(fromHex(value["arrears"].asString())));
        t.balance =  u256(fromBigEndian<u256>(fromHex(value["balance"].asString())));
        t.ballot =  u256(fromBigEndian<u256>(fromHex(value["ballot"].asString())));
        t.nonce =  u256(fromBigEndian<u256>(fromHex(value["nonce"].asString())));
        t.poll =  u256(fromBigEndian<u256>(fromHex(value["poll"].asString())));
    }

    u256 string_to_u256(const std::string &str) {
        return u256(fromBigEndian<u256>(fromHex(str)));
    }
}