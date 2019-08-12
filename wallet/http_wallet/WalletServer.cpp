//
// Created by lzb on 19-8-10.
//

#include "WalletServer.h"
#include "ToolTransaction.h"

wallet::WalletServer::WalletServer(HttpServer &server, std::string _send_url):AbstractServer<WalletServer>(server), m_send_url(_send_url){
//    this->bindAndAddMethod(Procedure("testhello", PARAMS_BY_NAME, JSON_STRING,
//                                     "test", JSON_STRING, NULL),
//                           &WalletServer::testhello);

    this->bindAndAddMethod(Procedure("sign_transaction", PARAMS_BY_NAME, JSON_STRING,
                                     "param", JSON_OBJECT, NULL),
                           &WalletServer::sign_transaction);

    this->bindAndAddMethod(Procedure("sign_transaction_send", PARAMS_BY_NAME, JSON_STRING,
                                     "param", JSON_OBJECT, NULL),
                           &WalletServer::sign_transaction_send);

}

void wallet::WalletServer::sign_transaction(const Json::Value &request, Json::Value &respone) {
    try {
        std::string json_str = request["param"].toStyledString();
        std::string _hash;
        std::pair<bool, std::string> _pair = ToolTransaction::sign_trx_from_json(json_str, _hash);

        if (_pair.first){
            respone["rlp"] = _pair.second;
            respone["hash"] = _hash;
            respone["isSend"] = false;
        }
        else{
            respone["error"] = _pair.second;
        }
    }
    catch (...){
        respone["error"] = "jsonrpccpp params error!";
    }

}

void wallet::WalletServer::sign_transaction_send(const Json::Value &request, Json::Value &respone) {

    std::pair<bool , std::string> _pair;
    std::string _hash;
    try {
        std::string json_str = request["param"].toStyledString();
        _pair = ToolTransaction::sign_trx_from_json(json_str, _hash);
    }
    catch (...){
        respone["error"] = "jsonrpccpp params error!";
        return;
    }
    try {
        if (!_pair.first){
            std::cout << "sing false"<<std::endl;
            respone["error"] = _pair.second;
        }
        else if (!m_send_url.empty()){
            std::cout << "to send:" << m_send_url<<" rlp:"<< _pair.second<< std::endl;
            respone["rlp"] = _pair.second;
            respone["hash"] = _hash;
            std::string _str=ToolTransaction::sendRawTransation(_pair.second, m_send_url);
            Json::Reader reader;
            Json::Value value;
            if (reader.parse(_str, value)) {            // json字符串转为json对象
                respone["sendRet"] = value;
            }
            else{
                respone["isSend"] = false;
                respone["sendRet"] = "send error , check the send_url";
            }
        }
        else{
            std::cout << "error: not has url"<<std::endl;
            respone["rlp"] = _pair.second;
            respone["hash"] = _hash;
            respone["isSend"] = false;
        }
    }
    catch (...){
        respone["error"] = "send field check the send_url";
    }


}

void wallet::WalletServer::testhello(const Json::Value &request, Json::Value &respone){
    std::cout<< "test:"<< request["test"].asString()<<  std::endl;
    respone = "test..." + request["test"].asString();
}
