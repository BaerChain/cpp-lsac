
#include "HttpWallet.h"
#include <libbrccore/ToolTransaction.h>
#include <libbrccore/CommonJS.h>

using namespace dev;
using namespace wallet;
Json::Value dev::wallet::HttpWallet::sign_transaction(Json::Value const&_param) {
    Json::Value respone;
    try {
        std::string _hash;
        cnote << "get message:" << " sign_transaction :";
        std::pair<bool, std::string> _pair = ToolTransaction::sign_trx_from_json(_param.toStyledString(), _hash);
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
    return respone;
}

Json::Value dev::wallet::HttpWallet::sign_transaction_send(Json::Value const&_param) {
    std::pair<bool , std::string> _pair;
    std::string _hash;
    Json::Value respone;
    try {
        cnote << "get message: "<< " sign_transaction_send :";
        _pair = ToolTransaction::sign_trx_from_json(_param.toStyledString(), _hash);
    }
    catch (...){
        cnote << "error : jsonrpccpp params error!";
        respone["error"] = "jsonrpccpp params error!";
        return respone;
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
    return  respone;
}

Json::Value dev::wallet::HttpWallet::new_address(std::string const &_param) {
    Json::Value respone;
    try {
        int create_number = std::atol(_param.data());

        if(create_number <= 0){
            respone["error"] = "numberm must > 0.";
            return respone;
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
                value["new-address"] = jsToNewAddress(key_pair.address());

                result.append(value);
            }
        }

        respone["result"] = result;
    }catch (...){
        respone["error"] = "invalid format";
    }
    return respone;
}
