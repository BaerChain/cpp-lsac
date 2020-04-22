//
// Created by lzb on 19-8-10.
//

#include "ToolTransaction.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <libbrccore/Common.h>
#include <libbrcdchain/Transaction.h>
#include <libdevcore/Address.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/base58.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <brc/types.hpp>


#define DATA_KEY_FROM "from"
#define DATA_KEY_TO "to"
#define DATA_KEY_NONCE "nonce"
#define DATA_KEY_DATA "data"
#define DATA_KEY_VALUE "value"
#define DATA_KEY_GAS "gas"
#define DATA_KEY_Price "gasPrice"
#define DATA_KEY_ChainId "chainId"

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

std::pair<bool, std::string> wallet::ToolTransaction::sign_trx_from_json(std::string json_str, std::string& transaction_hash) {
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
                tx.from = Address(d_obj[DATA_KEY_FROM].get_str());
                tx.to = Address(d_obj[DATA_KEY_TO].get_str());
                tx.value = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_VALUE].get_str())));
                tx.nonce = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_NONCE].get_str())));
                tx.gas = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_GAS].get_str())));
                tx.gasPrice = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_Price].get_str())));
                tx.chainId = u256(fromBigEndian<u256>(
                        fromHex(d_obj[DATA_KEY_ChainId].get_str())));   // must same with genesis chainId
                for (auto &p : d_obj[DATA_KEY_DATA].get_array()) {
                    auto op_obj = p.get_obj();
                    auto type = op_obj["type"].get_int();

                    if((op_type)type == op_type::deployContract){
                        tx.to = Address();
                        tx.isContract = trx_source::Contract::deploy;
                        tx.data = fromHex(op_obj["contract"].get_str());
                    } else if((op_type)type == op_type::executeContract){
                        tx.data = fromHex(op_obj["contract"].get_str());
                        tx.isContract = trx_source::Contract::execute;
                    } else {
                        auto op_ptr = get_oparation_from_data(op_obj, op_type::null);
                        if (!op_ptr) {
                            _pair.second = "can't get transaction type from json";
                            return _pair;
                        }
                        tx.ops.push_back(std::shared_ptr<operation>(op_ptr));
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
         catch (InvalidTransaciontType const& ex){
             _pair.second = "Error ";
             if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
                 _pair.second += std::string(*_error);
             return _pair;
         }
         catch (Exception& e){
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

    ///判断 有且只有一笔transction
    if(trx_datas.size() != 1) {
        std::string log = "error transction size must be 1";
        cerror << log;
        _pair.second = log;
        return _pair;
    }
    /// 获取交易子账户
    std::vector<Address> childAddress;
    if (obj.count("childAddresses")) {
        try {
            auto addr_array = obj["childAddresses"].get_array();
            for (auto &o : addr_array) {
                childAddress.emplace_back(Address(o.get_str()));
            }
        }
        catch (...) {
            _pair.second = "the childAddresses or invalid address";
            return _pair;
        }
    }
    /// 子账户签名
    for(auto& op : trx_datas[0].ops){
        if(!op){
            cwarn << " not have data in transaction";
            break;
        }
       auto d_type = op->type();
       if(d_type == op_type::transferMutilSigns){
           std::shared_ptr<transationTool::transferMutilSigns_operation> op_ptr =
                   std::dynamic_pointer_cast<transationTool::transferMutilSigns_operation> (op);
           if(op_ptr->m_data_ptrs.empty()){
               _pair.second = "in this mutilSignsTransaction not have transaction_data!";
               cerror << _pair.second;
               return _pair;
           }
           auto op_rlp = op_ptr->datasBytes();
           for(auto const&a : childAddress){
               if(!keys.count(a)) {
                   _pair.second = "not find the Child Addrss:"+ dev::toString(a) +" key...";
                   cwarn<< _pair.second;
                   return _pair;
               }
               auto sig = ToolTransaction::getSignByBytes(op_rlp, keys[a]);
               op_ptr->m_signs.emplace_back(sig);
           }
       }
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

std::pair<bool, std::string> wallet::ToolTransaction::sing_data_from_json(std::string json_str, std::string& transaction_hash){
    std::pair<bool , std::string> _pair = {false, ""};
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

    // load key
    Secret se_key;
    try{
        auto key = obj["key"].get_str();
        if(key.size() == 66 || key.size() == 64) {
            auto keyPair = dev::KeyPair(dev::Secret(key));
            se_key = keyPair.secret();
        }
        else {
            auto keyPair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(key)));
            se_key = keyPair.secret();
        }
    }
    catch (...){
        _pair.second = "Secret key format error";
        return  _pair;
    }
    return _pair;

    // load data
    std::vector<std::shared_ptr<operation>> ops;
    try{
        for(auto const& d : obj["data"].get_array()){
            auto data = d.get_obj();
            auto op_ptr = get_oparation_from_data(data, op_type::null);
            if (!op_ptr) {
                _pair.second = "can't get transaction type from json";
                return _pair;
            }
            ops.emplace_back(std::shared_ptr<operation>(op_ptr));
        }
    }
    catch (...)
    {
        _pair.second = "the json data format error";
        return  _pair;
    }

    auto rlp = packed_operation_data(ops);
    auto signs = getSignByBytes(rlp, se_key);
    //_pair.second =getSignByBytes(rlp, se_key);  //dev::toHex(rlp);
    _pair.first = true;
    RLPStream s(3);
    s << (u256)signs.r << (u256)signs.s << signs.v;
    _pair.second = dev::toHex(s.out());
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
        _v.push_back(p->serialize());
    }
    rlp.appendVector<bytes>(_v);
    return rlp.out();
}

SignatureStruct wallet::ToolTransaction::getSignByBytes(bytes const& _bs, Secret _key){
    auto sig = dev::sign(_key,dev::sha3(_bs));
    SignatureStruct sigStruct = *(SignatureStruct const*)&sig;
    if (sigStruct.isValid())
        return sigStruct;
    return SignatureStruct{h256(),h256(),0};
}

operation* wallet::ToolTransaction::get_oparation_from_data(js::mObject& op_obj, op_type parent_type){
//    if(!(parent_type==op_type::null || parent_type ==op_type::transferMutilSigns))
//        BOOST_THROW_EXCEPTION(InvalidTransaciontType()<<errinfo_comment("transaction type:"+std::to_string(parent_type)+" can't have child"));
    auto type = op_obj["type"].get_int();
    switch (type) {
        case vote: {
            auto new_op = new vote_operation((op_type) type,
                                             Address(op_obj["m_from"].get_str()),
                                             Address(op_obj["m_to"].get_str()),
                                             (uint8_t) op_obj["m_vote_type"].get_int(),
                                             u256(op_obj["m_vote_numbers"].get_str())
            );
            return new_op;
        }
        case brcTranscation: {
            auto transcation_op = new transcation_operation((op_type) type,
                                                            Address(op_obj["m_from"].get_str()),
                                                            Address(op_obj["m_to"].get_str()),
                                                            (uint8_t) op_obj["m_transcation_type"].get_int(),
                                                            u256(op_obj["m_transcation_numbers"].get_str())
            );
            return transcation_op;
        }
        case pendingOrder: {
            auto pendingorder_op = new pendingorder_opearaion((op_type) type,
                                                              Address(op_obj["m_from"].get_str()),
                                                              (ex::order_type) op_obj["m_type"].get_int(),
                                                              (ex::order_token_type) op_obj["m_token_type"].get_int(),
                                                              (ex::order_buy_type) op_obj["m_buy_type"].get_int(),
                                                              u256(op_obj["m_num"].get_str()),
                                                              u256(op_obj["m_price"].get_str())
            );
            return pendingorder_op;
        }
        case cancelPendingOrder: {
            auto cancel_op = new cancelPendingorder_operation((op_type) type, 3, h256(op_obj["m_hash"].get_str()));
            return cancel_op;
        }
        case deployContract: {
            if(parent_type == op_type::null)
                BOOST_THROW_EXCEPTION(InvalidTransaciontType()<<errinfo_comment("this transaction can't have type:5"));
            return new contract_operation((op_type)type, Address(), fromHex(op_obj["contract"].get_str()));
        }
        case executeContract: {
//            if(parent_type == op_type::null)
//                BOOST_THROW_EXCEPTION(InvalidTransaciontType()<<errinfo_comment("this transaction can't have type:6"));
            return new contract_operation((op_type)type, Address(op_obj["m_to"].get_str()), fromHex(op_obj["contract"].get_str()));
        }
        case changeMiner: {
//            if(parent_type == op_type::transferMutilSigns)
//                BOOST_THROW_EXCEPTION(InvalidTransaciontType()<<errinfo_comment("transaction type:11 cant't contains type:7"));
            auto changeMiner_op = new changeMiner_operation( (op_type)type,
                                                             Address(op_obj["m_before"].get_str()),
                                                             Address(op_obj["m_after"].get_str())
            );
            return changeMiner_op;
        }
        case receivingincome: {
            auto receivingincome_op = new receivingincome_operation((op_type)type,
                                                            (uint8_t)op_obj["m_receivingType"].get_int(),
                                                            Address(op_obj["m_from"].get_str()));
            return receivingincome_op;
        }
        case transferAutoEx: {
            auto transferAutoEx_op = new transferAutoEx_operation(
                    (op_type)type,
                    transferAutoExType(op_obj["m_autoExType"].get_int()),
                    u256(op_obj["m_autoExNum"].get_str()),
                    u256(op_obj["m_transferNum"].get_str()),
                    Address(op_obj["m_from"].get_str()),
                    Address(op_obj["m_to"].get_str()));
            return transferAutoEx_op;
        }
        case transferAccountControl:{
//            if(parent_type == op_type::transferMutilSigns)
//                BOOST_THROW_EXCEPTION(InvalidTransaciontType()<<errinfo_comment("transaction type:11 cant't contains type:10"));
            auto authority_op = new authority_operation(
                    (op_type)type,
                    Address(op_obj["childAddress"].get_str()),
                    uint8_t (op_obj["weight"].get_int()),
                    uint8_t (op_obj["permissions"].get_int())
                    );
            return authority_op;
        }
        case transferMutilSigns: {
            if(parent_type == op_type::transferMutilSigns) {
                BOOST_THROW_EXCEPTION(InvalidTransaciontType()<<errinfo_comment("transaction type:11 can't nested"));
            }
            auto txData = op_obj["transactionData"].get_array();
            std::vector<operation*> ptrs;
            for(auto &d : txData){
                auto op_ptr = get_oparation_from_data(d.get_obj(), op_type::transferMutilSigns);
                if(!op_ptr)
                    BOOST_THROW_EXCEPTION(InvalidTransaciontType()<<errinfo_comment("transaction type:11 can't contains empty"));
                ///verify operation type is allowed mutilSign
                authority::getPermissionsTypeByTransactionType(op_ptr->type());
                ptrs.emplace_back(op_ptr);
            }
            auto tran_sings = new transferMutilSigns_operation((op_type)type,Address(op_obj["rootAddress"].get_str()),
                                                        Address(op_obj["cookiesAddress"].get_str()),ptrs);
            return tran_sings;
        }
        case authorizeUseCookie:{
            auto authorize_op = new authorizeCookies_operation((op_type) type,
                                                              authorizeCookieType(op_obj["m_authorizeType"].get_int()),
                                                              Address(op_obj["m_childAddr"].get_str())
            );
            return authorize_op;
        }
        default:
            BOOST_THROW_EXCEPTION(InvalidTransaciontType()<<errinfo_comment(" Invalid transaction type"));
            break;
    }
    return nullptr;
}

