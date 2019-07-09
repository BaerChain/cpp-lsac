//
// Created by friday on 22/05/2019.
//

#pragma once

#include <json/value.h>
#include <json/reader.h>
#include <string>
#include <iostream>
#include <libdevcore/Address.h>
#include <libbrccore/Common.h>

using namespace dev;


namespace test_tools{

    struct request_rpc{
        std::string jsonrpc;
        std::string id;
        Json::Value result;
    };


    struct account{
        dev::Address    address;
        dev::u256       BRC;
        dev::u256       FBRC;
        dev::u256       FBalance;
        dev::u256       arrears;
        dev::u256       balance;
        dev::u256       ballot;
        dev::u256       nonce;
        dev::u256       poll;

    };


    std::string post_message(std::string const &_rlpStr, std::string const &_ip_port);



    Json::Value string_to_json(const std::string &source);


    u256    string_to_u256(const std::string &str);

    template<typename T>
    void to_variant(const Json::Value &value, T &t);

    template <>
    void to_variant<request_rpc>(const Json::Value &value, request_rpc &t);

    template <>
    void to_variant<account>(const Json::Value &value, account &t);


}