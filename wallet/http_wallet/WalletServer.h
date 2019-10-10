//
// Created by lzb on 19-8-10.
//
#ifndef BAERCHAIN_WALLETSERVER_H
#define BAERCHAIN_WALLETSERVER_H

#include <iostream>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/connectors/httpserver.h>

using namespace jsonrpc;
using namespace std;

namespace wallet{

class WalletServer : public jsonrpc::AbstractServer<WalletServer> {
public:
    WalletServer(HttpServer &server, std::string _send_url = "");
    void testhello(const Json::Value& request, Json::Value & respone);
    /// transaction sign and send to chain return send_ret
    void sign_transaction_send(const Json::Value& request, Json::Value & respone);

    /// transaction sign and return sing_data
    void sign_transaction(const Json::Value & request, Json::Value & respone);

    /// new address by
    void new_address(const  Json::Value & request, Json::Value & respone);

    bool test_connect_node();

private:
    std::string m_send_url;

};

}



#endif //BAERCHAIN_WALLETSERVER_H
