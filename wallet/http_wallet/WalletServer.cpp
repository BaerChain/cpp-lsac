//
// Created by lzb on 19-8-10.
//

#include "WalletServer.h"
#include "ToolTransaction.h"

wallet::WalletServer::WalletServer(dev::SafeHttpServer &server, std::string _send_url):AbstractServer<WalletServer>(server), m_send_url(_send_url){
//    this->bindAndAddMethod(Procedure("testhello", PARAMS_BY_NAME, JSON_STRING,
//                                     "test", JSON_STRING, NULL),
//                           &WalletServer::testhello);

    this->bindAndAddMethod(Procedure("sign_transaction", PARAMS_BY_NAME, JSON_STRING,
                                     "param", JSON_OBJECT, NULL),
                           &WalletServer::sign_transaction);

    this->bindAndAddMethod(Procedure("sign_transaction_send", PARAMS_BY_NAME, JSON_STRING,
                                     "param", JSON_OBJECT, NULL),
                           &WalletServer::sign_transaction_send);

//    this->bindAndAddMethod(Procedure("new_address", PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL), &WalletServer::new_address);

    this->bindAndAddMethod(Procedure("new_address", PARAMS_BY_POSITION, JSON_OBJECT, "param1", JSON_STRING, NULL),
                           &WalletServer::new_address);
}

void wallet::WalletServer::sign_transaction(const Json::Value &request, Json::Value &respone) {
    try {
        std::string json_str = request["param"].toStyledString();
        std::string _hash;
        cnote << "get message:" << " sign_transaction :";
        std::pair<bool, std::string> _pair = ToolTransaction::sign_trx_from_json(json_str, _hash);

        if (_pair.first){
            respone["rlp"] = _pair.second;
            respone["hash"] = _hash;
            respone["isSend"] = false;
            cnote << "sign transaction:"<< respone["hash"] << "rlp:"<< respone["rlp"];
        }
        else{
            cerror << "error : sign field:"<< _pair.second;
            respone["error"] = _pair.second;
        }
    }
    catch (...){
        cnote << "error : jsonrpccpp params error!";
        respone["error"] = "jsonrpccpp params error!";
    }

}

void wallet::WalletServer::sign_transaction_send(const Json::Value &request, Json::Value &respone) {

    std::pair<bool , std::string> _pair;
    std::string _hash;
    try {
        std::string json_str = request["param"].toStyledString();
        cnote << "get message: "<< " sign_transaction_send :";
        _pair = ToolTransaction::sign_trx_from_json(json_str, _hash);
    }
    catch (...){
        cnote << "error : jsonrpccpp params error!";
        respone["error"] = "jsonrpccpp params error!";
        return;
    }
    try {
        if (!_pair.first){
            cerror << "error sing false :"<< _pair.second;
            respone["error"] = _pair.second;
        }
        else if (!m_send_url.empty()){
            cnote << "will to send:" << m_send_url<<" rlp:"<< _pair.second;
            respone["rlp"] = _pair.second;
            respone["hash"] = _hash;
            std::string _str=ToolTransaction::sendRawTransation(_pair.second, m_send_url);
            Json::Reader reader;
            Json::Value value;
            if (reader.parse(_str, value)) {            // json字符串转为json对象
                cnote << "send success..." << _str;
                respone["sendRet"] = value;
            }
            else{
                cerror << " send field: check the jsonData";
                respone["isSend"] = false;
                respone["sendRet"] = "send error , check the jsonData";
            }
        }
        else{
            cnote << "sign data:" << _pair.second;
            cerror << "error: not has url to send";
            respone["rlp"] = _pair.second;
            respone["hash"] = _hash;
            respone["isSend"] = false;
        }
    }
    catch (...){
        cerror << "send field check the send_url or network";
        respone["error"] = "send field check the send_url or network";
    }


}

void wallet::WalletServer::testhello(const Json::Value &request, Json::Value &respone){
    cnote<< "test:"<< request["test"].asString();
    respone = "test..." + request["test"].asString();
}

void wallet::WalletServer::new_address(const Json::Value & request, Json::Value & respone) {

    try {
        int create_number = std::atol(request[0u].asString().data());

        if(create_number <= 0){
            respone["error"] = "numberm must > 0.";
            return;
        }
        Json::Value result;

        if(create_number > 0){
            for(int i = 0 ; i < create_number; i++){
                auto key_pair = KeyPair::create();
                auto sec = key_pair.secret();

                Json::Value value;
                value["pri-key-base58"] = dev::crypto::to_base58((char*)sec.data(), 32);
                value["pri-key-bigInt"] = toHex(sec.ref());
                value["address"] = toHex(key_pair.address());
                value["pub-key"] = toHex(key_pair.pub());

                result.append(value);
            }
        }

        respone["result"] = result;
    }catch (...){
        respone["error"] = "invalid format";
    }

}

bool wallet::WalletServer::test_connect_node() {
    if (m_send_url.empty()){
        cnote<< "Warning the send url is empty!";
    }
    try {
        std::string _str=ToolTransaction::connectNode( m_send_url);
        Json::Reader reader;
        Json::Value value;
        if (reader.parse(_str, value)) {            // json字符串转为json对象
            if(value.isMember("result")) {
                cnote << "connect host:"<< m_send_url<< " is ok!";
                return true;
            }
        }
    }
    catch (...){
    }
    cnote << "Warning can not connect host:"<< m_send_url;
    cnote << "can not to use method: sign_transaction_send";
    return false;
}
