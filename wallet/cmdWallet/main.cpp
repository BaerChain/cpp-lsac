//
// Created by fri on 2019/3/20.
//


#include <boost/program_options.hpp>
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

namespace bpo = boost::program_options;
namespace bfs1 = boost::filesystem;
using namespace dev::brc;

// bool creation = false;
//	Address from;
//	Address to;
//	u256 value;
//	bytes data;
//	u256 nonce = Invalid256;
//	u256 gas = Invalid256;
//	u256 gasPrice = Invalid256;


#define DATA_KEY_FROM "from"
#define DATA_KEY_TO "to"
#define DATA_KEY_NONCE "nonce"
#define DATA_KEY_DATA "data"
#define DATA_KEY_VALUE "value"
#define DATA_KEY_GAS "gas"
#define DATA_KEY_Price "gasPrice"


namespace js = json_spirit;

using namespace dev;
using namespace dev::brc::transationTool;
static size_t nonce = 0;

bool validate_obj(const json_spirit::mObject &obj) {
    requireJsonFields(obj, "data",
                      {{DATA_KEY_FROM,  {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_TO,    {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_VALUE, {{js::str_type}, JsonFieldPresence::Optional}},
                       {DATA_KEY_DATA,  {{js::obj_type}, JsonFieldPresence::Optional}},
                       {DATA_KEY_NONCE, {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_GAS,   {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_Price, {{js::str_type}, JsonFieldPresence::Required}}}

    );
    return true;
}






struct trx_source {
    enum Contract
	{
        null =0,
        deploy =1,
        execute
	};
    Address from;
    Address to;
    u256 value;
    bytes data;
    u256 nonce = Invalid256;
    u256 gas = Invalid256;
    u256 gasPrice = Invalid256;
    std::vector<std::shared_ptr<operation>> ops;
	Contract isContract = Contract::null;
};





bytes packed_operation_data(const std::vector<std::shared_ptr<operation>> &op) {
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




u256 get_address_nonce(const Address &add, const std::string &ip){
    try {
        std::string _result;
        std::string send_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_getBalance\",\"params\":[\""+ toHex(add)  +"\", \"-1\"],\"id\":1}";
        jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(ip);
        _httpClient.SendRPCMessage(send_msg, _result);
        auto value = string_to_json(_result)["result"];
        return u256(fromBigEndian<u256>(fromHex(value["nonce"].asString())));
    }catch (...){

    }


    return 0;
}

void sendRawTransation(std::string const &_rlpStr, std::string const &_ip_port) {
    std::string _result;

    std::string send_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_sendRawTransaction\",\"params\":[\"" + _rlpStr +"\"],\"id\":1}";

//    cerror << "send message " << send_msg << std::endl;

    jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(_ip_port);
    _httpClient.SendRPCMessage(send_msg, _result);

    cwarn << _result;

}

bool sign_trx_from_json(const bfs1::path &path, bool _is_send, std::string _ip = "") {
    try {
        js::mValue val;
        js::read_string_or_throw(contentsString(path.string()), val);
        js::mObject obj = val.get_obj();

        std::vector<trx_source> trx_datas;
        //获取数据
        if (obj.count("source")) {
            auto array = obj["source"].get_array();
            for (auto &data : array) {
				trx_source tx;
                auto d_obj = data.get_obj();
                tx.from = Address(d_obj[DATA_KEY_FROM].get_str());
                tx.to = Address(d_obj[DATA_KEY_TO].get_str());
                tx.value = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_VALUE].get_str())));
                tx.nonce = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_NONCE].get_str())));
                tx.gas = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_GAS].get_str())));
                tx.gasPrice = u256(fromBigEndian<u256>(fromHex(d_obj[DATA_KEY_Price].get_str())));
                for (auto &p : d_obj[DATA_KEY_DATA].get_array()) {
                    auto op_obj = p.get_obj();
                    auto type = op_obj["type"].get_int();
                    switch (type) {
                        case vote: {
                            auto new_op = new vote_operation((op_type) type,
                                                             Address(op_obj["m_from"].get_str()),
                                                             Address(op_obj["m_to"].get_str()),
                                                             (uint8_t) op_obj["m_vote_type"].get_int(),
                                                             op_obj["m_vote_numbers"].get_int()
                                                             );
                            tx.ops.push_back(std::shared_ptr<vote_operation>(new_op));
                            break;
                        }
                        case brcTranscation: {
                            auto transcation_op = new transcation_operation((op_type) type,
                                                                            Address(op_obj["m_from"].get_str()),
                                                                            Address(op_obj["m_to"].get_str()),
                                                                            (uint8_t) op_obj["m_transcation_type"].get_int(),
                                                                            u256(fromBigEndian<u256>(fromHex(op_obj["m_transcation_numbers"].get_str())))
                                                                            );
                            tx.ops.push_back(std::shared_ptr<transcation_operation>(transcation_op));
                            break;
                        }
                        case pendingOrder: {
                            auto pendingorder_op = new pendingorder_opearaion( (op_type)type,
                                                                              Address(op_obj["m_from"].get_str()),
                                                                              (ex::order_type) op_obj["m_type"].get_int(),
                                                                              (ex::order_token_type) op_obj["m_token_type"].get_int(),
                                                                              (ex::order_buy_type) op_obj["m_buy_type"].get_int(),
                                                                               u256(fromBigEndian<u256>(fromHex(op_obj["m_num"].get_str()))),
                                                                               u256(fromBigEndian<u256>(fromHex(op_obj["m_price"].get_str())))
                                                                              );
                            tx.ops.push_back(std::shared_ptr<pendingorder_opearaion>(pendingorder_op));
                            break;
                        }
						case cancelPendingOrder: {
							auto cancel_op = new cancelPendingorder_operation((op_type)type, 3,h256(op_obj["m_hash"].get_str()));
							tx.ops.push_back(std::shared_ptr<cancelPendingorder_operation>(cancel_op));
							break;
						}
						case deployContract: {
							tx.to = Address();
							tx.isContract = trx_source::Contract::deploy ;
							tx.data = fromHex(op_obj["contract"].get_str());
                            break;
						}
                        case executeContract:{						
							tx.data = fromHex(op_obj["contract"].get_str());
							tx.isContract = trx_source::Contract::execute;
                            break;
						}     
					}
                }
                trx_datas.push_back(tx);
            }
        } else {
            std::cout << "not find source.\n";
            exit(1);
        }

        //获取私钥

        std::map<Address, Secret> keys;
        if (obj.count("keys")) {
            for (auto &obj : obj["keys"].get_array()) {
                auto key = obj.get_str();
                auto keyPair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(key)));
                keys[keyPair.address()] = keyPair.secret();
            }
        } else {
            exit(1);
        }

        //数据签名
        std::vector<brc::Transaction> sign_trxs;
        for (auto &t : trx_datas) {
            if (keys.count(t.from)) {
				brc::TransactionSkeleton ts;
                if(t.isContract == trx_source::Contract::null)
				{
					bytes data = packed_operation_data(t.ops);
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
                ts.from = t.from;
                ts.to = t.to;
                ts.value = t.value;
                ts.nonce = get_address_nonce(t.from, _ip);

                ts.gas = t.gas;
                ts.gasPrice = t.gasPrice;

                brc::Transaction sign_t(ts, keys[t.from]);

//                auto sssss = dev::brc::toJson(sign_t);
//                cerror << "test: " << sssss << std::endl;
                if (_is_send) {
                    sendRawTransation(toHexPrefixed(sign_t.rlp()), _ip);
                }
            } else {
                std::cout << "please input address: " << t.from << " private key.";
                exit(1);
            }
        }


        return true;
    }
    catch (const std::exception &e) {
        std::cout << "exception : " << e.what() << std::endl;
    }
    catch (const boost::exception &e) {
        std::cout << "xxxxxxx " << std::endl;
    }

    return false;
}


void write_simple_to_file(const bfs1::path &path) {
    const std::string simple =
            "{\n"
            "  \"source\": [\n"
            "    {\n"
            "      \"from\": \"e523e7c59a0725afd08bc9751c89eed6f8e16dec\",\n"
            "      \"to\": \"e523e7c59a0725afd08bc9751c89eed6f8e16dec\",\n"
            "      \"value\": \"0x1388\",\n"
            "      \"data\": [\n"
            "        {\n"
            "          \"m_type\": 1,\n"
            "          \"m_from\": \"e523e7c59a0725afd08bc9751c89eed6f8e16dec\",\n"
            "          \"m_to\": \"e523e7c59a0725afd08bc9751c89eed6f8e16dec\",\n"
            "          \"m_vote_type\": 1,\n"
            "          \"m_vote_numbers\": 1\n"
            "        }\n"
            "      ],\n"
            "      \"nonce\": \"0x1388\",\n"
            "      \"gas\": \"0x1388\",\n"
            "      \"gasPrice\": \"0x1388\"\n"
            "    }\n"
            "  ],\n"
            "  \"keys\": [\n"
            "    \"8RioSGhgNUKFZopC2rR3HRDD78Sc48gci4pkVhsduZve\"\n"
            "  ]\n"
            "}";
    std::cout << simple << std::endl;
    writeFile(path, simple);
}



void generate_key(const std::string &seed){
    auto key_pair = KeyPair::create();
    auto sec = key_pair.secret();
    std::cout << "private key: " << dev::crypto::to_base58((char*)sec.data(), 32) << std::endl;
    std::cout << "address : " << key_pair.address() << std::endl;

}


int main(int argc, char *argv[]) {

    try {
        bpo::options_description description("command line ");
        description.add_options()
                ("help,h", "show help message.")
                ("json,j", bpo::value<bfs1::path>(), "read from data from file.")
                ("send,s", bpo::value<std::string>(), "get the http ip and port, use this option will auto to send rawTransation to http host...")
                ("nonce,n", bpo::value<int>(), "set the transation nonce ....")
                ("create,c", "create simple \"data.json\" to file on current path.")
                ("generate-key,g", bpo::value<std::string>(),"by seed generate private-key and address. ")
                ;
        // addNetworkingOption("listen-ip", po::value<string>()->value_name("<ip>(:<port>)"),
        //"Listen on the given IP for incoming connections (default: 0.0.0.0)");

        bpo::variables_map args_map;
        bpo::parsed_options parsed = bpo::parse_command_line(argc, argv, description);
        bpo::store(parsed, args_map);

        bfs1::path json_path;

        if (args_map.count("help")) {
            std::cout << description << std::endl;
            return 0;
        }
        if (args_map.count("create")) {
            auto p = bfs1::current_path().string() + "/data.json";
            write_simple_to_file(bfs1::path(p));
        }

        bool _is_send = false;
        std::string _ip = "";
        if (args_map.count("send")) {
            _is_send = true;
            _ip = args_map["send"].as<std::string>();
        }
        if (args_map.count("json")) {
            json_path = args_map["json"].as<bfs1::path>();
            sign_trx_from_json(json_path, _is_send, _ip);
            return 0;
        }
        if (args_map.count("nonce")) {
            nonce = (size_t) args_map["nonce"].as<int>();
        }
        if (args_map.count("generate-key")) {
            generate_key(args_map["generate-key"].as<std::string>());
        }
    }catch (const std::exception &e){
        cwarn << e.what();
    }catch (const boost::exception &e){
        cwarn << boost::diagnostic_information(e);
    }catch (...){

    }




    return 0;
}
