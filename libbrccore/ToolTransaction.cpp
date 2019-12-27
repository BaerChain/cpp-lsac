//
// Created by lzb on 19-8-10.
//

#include "ToolTransaction.h"
#include <cstdlib>
#include <json_spirit/JsonSpiritHeaders.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <libbrccore/Common.h>
#include <libbrcdchain/Transaction.h>
#include <libdevcore/Address.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
#include <libdevcore/JsonUtils.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/base58.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <libweb3jsonrpc/JsonHelper.h>
#include <libbrccore/CommonJS.h>
#include <brc/types.hpp>


#define DATA_KEY_FROM "from"
#define DATA_KEY_TO "to"
#define DATA_KEY_NONCE "nonce"
#define DATA_KEY_DATA "data"
#define DATA_KEY_VALUE "value"
#define DATA_KEY_GAS "gas"
#define DATA_KEY_Price "gasPrice"
#define DATA_KEY_ChainId "chainId"

using namespace dev;

std::string wallet::ToolTransaction::sendRawTransation(std::string const &_rlpStr, std::string const &_ip_port) {
    // toHexPrefixed(sign_t.rlp())
    std::string _result;
    std::string send_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_sendRawTransaction\",\"params\":[\"" + _rlpStr +"\"],\"id\":1}";
    jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(_ip_port);
    _httpClient.SetTimeout(1000);
    _httpClient.SendRPCMessage(send_msg, _result);
    return _result;
}

std::string wallet::ToolTransaction::connectNode(std::string const& _ip_port) {

    std::string _result;
    std::string send_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_protocolVersion\",\"params\":[],\"id\":1}";
    jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(_ip_port);
    _httpClient.SetTimeout(1000);
    _httpClient.SendRPCMessage(send_msg, _result);
    return _result;
}

std::pair<bool, std::string> wallet::ToolTransaction::sign_trx_from_json(std::string const& json_str, std::string& transaction_hash,
                                                                         std::string const& _get_nonce_ip /*=""*/) {
    std::pair<bool , std::string> _pair =  std::make_pair(false, "");
    js::mValue val;
    js::mObject obj;
    try {
        js::read_string_or_throw(json_str, val);
        obj = val.get_obj();
    }
    catch (...){
        _pair.second = "the json format error";
        return _pair;
    }
    std::vector<trx_source> trx_datas;
        //获取数据
     if (obj.count("source")) {
         try{
            auto array = obj["source"].get_array();
            if (array.empty()){
                cerror << "error the source is null !";
                _pair.second = "the source is null";
                return _pair;
            }
            for (auto &data : array) {
                trx_source tx;
                auto d_obj = data.get_obj();
                tx.from = jsToAddressAcceptAllAddress(d_obj[DATA_KEY_FROM].get_str());
                tx.to = jsToAddressAcceptAllAddress(d_obj[DATA_KEY_TO].get_str());
                tx.value = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_VALUE].get_str())));
                tx.nonce = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_NONCE].get_str())));
                tx.gas = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_GAS].get_str())));
                tx.gasPrice = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_Price].get_str())));
                tx.chainId = u256(fromBigEndian<u256>(
                        fromHex(d_obj[DATA_KEY_ChainId].get_str())));   // must same with genesis chainId
                for (auto &p : d_obj[DATA_KEY_DATA].get_array()) {
                    auto op_obj = p.get_obj();
                    auto type = op_obj["type"].get_int();
                    switch (type) {
                        case vote: {
                            auto new_op = new vote_operation((op_type) type,
                                                             jsToAddressAcceptAllAddress(op_obj["m_from"].get_str()),
                                                             jsToAddressAcceptAllAddress(op_obj["m_to"].get_str()),
                                                             (uint8_t) op_obj["m_vote_type"].get_int(),
                                                             u256(op_obj["m_vote_numbers"].get_str())
                            );
                            tx.ops.push_back(std::shared_ptr<vote_operation>(new_op));
                            break;
                        }
                        case brcTranscation: {
                            auto transcation_op = new transcation_operation((op_type) type,
                                                                            jsToAddressAcceptAllAddress(op_obj["m_from"].get_str()),
                                                                            jsToAddressAcceptAllAddress(op_obj["m_to"].get_str()),
                                                                            (uint8_t) op_obj["m_transcation_type"].get_int(),
                                                                            u256(op_obj["m_transcation_numbers"].get_str())
                            );
                            tx.ops.push_back(std::shared_ptr<transcation_operation>(transcation_op));
                            break;
                        }
                        case pendingOrder: {
                            auto pendingorder_op = new pendingorder_opearaion((op_type) type,
                                                                              jsToAddressAcceptAllAddress(op_obj["m_from"].get_str()),
                                                                              (ex::order_type) op_obj["m_type"].get_int(),
                                                                              (ex::order_token_type) op_obj["m_token_type"].get_int(),
                                                                              (ex::order_buy_type) op_obj["m_buy_type"].get_int(),
                                                                              u256(op_obj["m_num"].get_str()),
                                                                              u256(op_obj["m_price"].get_str())
                            );
                            tx.ops.push_back(std::shared_ptr<pendingorder_opearaion>(pendingorder_op));
                            break;
                        }
                        case cancelPendingOrder: {
                            auto cancel_op = new cancelPendingorder_operation((op_type) type, 3,
                                                                              h256(op_obj["m_hash"].get_str()));
                            tx.ops.push_back(std::shared_ptr<cancelPendingorder_operation>(cancel_op));
                            break;
                        }
                        case deployContract: {
                            tx.to = Address();
                            tx.isContract = trx_source::Contract::deploy;
                            tx.data = fromHex(op_obj["contract"].get_str());
                            break;
                        }
                        case executeContract: {
                            tx.data = fromHex(op_obj["contract"].get_str());
                            tx.isContract = trx_source::Contract::execute;
                            break;
                        }
                        case changeMiner: {
                            auto changeMiner_op = new changeMiner_operation((op_type) type,
                                                                            jsToAddressAcceptAllAddress(op_obj["m_before"].get_str()),
                                                                            jsToAddressAcceptAllAddress(op_obj["m_after"].get_str())
                            );
                            tx.ops.push_back(std::shared_ptr<changeMiner_operation>(changeMiner_op));
                            break;
                        }
                        case receivingincome: {
                            auto receivingincome_op = new receivingincome_operation((op_type) type,
                                                                                    1,
                                                                                    jsToAddressAcceptAllAddress(op_obj["m_from"].get_str())
                            );
                            tx.ops.push_back(std::shared_ptr<receivingincome_operation>(receivingincome_op));
                            break;
                        }
                        case transferAutoEx:{
                            auto transferAutoEx_op = new transferAutoEx_operation( 
                                                                                    (op_type)type,
                                                                                    transferAutoExType(op_obj["m_autoExType"].get_int()),
                                                                                    u256(op_obj["m_autoExNum"].get_str()),
                                                                                    u256(op_obj["m_transferNum"].get_str()),
                                                                                    jsToAddressAcceptAllAddress(op_obj["m_from"].get_str()),
                                                                                    jsToAddressAcceptAllAddress(op_obj["m_to"].get_str())
                            );
                            tx.ops.push_back(std::shared_ptr<transferAutoEx_operation>(transferAutoEx_op));
                            break;                    
                        }
                    }
                }
                trx_datas.push_back(tx);
            }

            Json::Reader reader ;
            Json::Value val;
            if(reader.parse(json_str, val)){
                cnote << "Analysis of the json_data success... json_data: \n"<<val["source"].toStyledString();
            }
         }
         catch (std::exception& e){
             cerror << "the json format error. ";
             cerror << e.what();
             //_pair.second = e.what();
             _pair.second = "the data_json Field type error";
             return _pair;
         }
         catch (...){
             _pair.second = "the data_json is error";
             return _pair;
         }

     } else {
         cerror << "not find source.\n";
            _pair.second = "json_data is error not has key:source";
         return _pair;
     }

    //获取私钥

    std::map<Address, Secret> keys;
    if (obj.count("keys")) {
        js::mArray key_array;
        try {
            key_array = obj["keys"].get_array();
        }
        catch (...){
            _pair.second = "the keys not is array";
            return  _pair;
        }
        try{
            for (auto &obj : key_array) {
                auto key = obj.get_str();
                if(key.size() == 66 || key.size() == 64) {
                    auto keyPair = dev::KeyPair(dev::Secret(key));
                    keys[keyPair.address()] = keyPair.secret();
                }
                else {
                    auto keyPair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(key)));
                    keys[keyPair.address()] = keyPair.secret();
                }
            }
        }
        catch (...){
            _pair.second = "Secret key format error";
            return  _pair;
        }

    } else {
        cerror << "not find key.....\n";
        _pair.second = "json_data is error not has key:keys";
        return _pair;
    }

    //数据签名
    std::vector<brc::Transaction> sign_trxs;
    for (auto &t : trx_datas) {
        if (keys.count(t.from)) {
            brc::TransactionSkeleton ts;
            if(t.isContract == trx_source::Contract::null)
            {
                bytes data = ToolTransaction::packed_operation_data(t.ops);
                ts.data = data;
            }
            else if(t.isContract == trx_source::Contract::deploy)
            {
                ts.data = t.data;
                ts.creation = true;
            }
            else if(t.isContract == trx_source::Contract::execute)
            {
                ts.data = t.data;
            }
            ts.chainId = t.chainId;
            ts.from = t.from;
            ts.to = t.to;
            ts.value = t.value;
            if(!_get_nonce_ip.empty()) {
                //ts.nonce = get_address_nonce(t.from, _ip);
                auto ret= get_address_nonce(t.from, _get_nonce_ip);
                if(ret.first){
                    ts.nonce = ret.second;
                } else{
                    ts.nonce = t.nonce;
                }
            }
            else
                ts.nonce = t.nonce;
            ts.gas = t.gas;
            ts.gasPrice = t.gasPrice;
            brc::Transaction sign_t(ts, keys[t.from]);

            _pair.second =toHexPrefixed(sign_t.rlp());
            _pair.first = true;
            transaction_hash =toHexPrefixed(sign_t.sha3());
        } else {
            cerror << "please input address: " << t.from << " private key.";
            _pair.second = "not find the addrss:"+ dev::toString(t.from) +" key...";
        }
    }
    return _pair;
}

bytes wallet::ToolTransaction::packed_operation_data(const std::vector<std::shared_ptr<operation>> &op)
{
    if (!op.size()) {
        return bytes();
    }
    RLPStream rlp;
    std::vector<bytes> _v;
    for (auto &p : op) {
        // rlp.append(p->serialize());
        _v.push_back(p->serialize());
    }
    rlp.appendVector<bytes>(_v);
    return rlp.out();
}

std::pair<bool, u256> wallet::ToolTransaction::get_address_nonce(const Address &add, const std::string &ip) {
    try {
        std::string _result;
        std::string send_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_getBalance\",\"params\":[\""+ toHex(add)  +"\", \"latest\"],\"id\":1}";
        jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(ip);
        _httpClient.SendRPCMessage(send_msg, _result);
        auto value = string_to_json(_result)["result"];
        return std::make_pair(true, u256(fromBigEndian<u256>(fromHex(value["nonce"].asString()))) );
    }catch (...){
    }
    return std::make_pair(false, 0);
}

Json::Value wallet::ToolTransaction::string_to_json(const std::string &source) {
    auto readerBuilder = Json::CharReaderBuilder();
    auto sreader = readerBuilder.newCharReader();
    Json::Value root;
    std::string errotStr;
    if (!sreader->parse(source.c_str(), source.c_str() + source.size(), &root, &errotStr)) {
        std::cout << "TODO throw exception.\n";
    }
    return root;
}

