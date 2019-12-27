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
#include <libbrccore/ToolTransaction.h>

namespace bpo = boost::program_options;
namespace bfs1 = boost::filesystem;
using namespace dev::brc;


#define DATA_KEY_FROM "from"
#define DATA_KEY_TO "to"
#define DATA_KEY_NONCE "nonce"
#define DATA_KEY_DATA "data"
#define DATA_KEY_VALUE "value"
#define DATA_KEY_GAS "gas"
#define DATA_KEY_Price "gasPrice"
#define DATA_KEY_ChainId "chainId"


namespace js = json_spirit;

using namespace dev;
using namespace dev::brc::transationTool;
static size_t nonce = 0;

bool validate_obj(const json_spirit::mObject &obj) {
    requireJsonFields(obj, "data",
                      {{DATA_KEY_FROM,      {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_TO,        {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_VALUE,     {{js::str_type}, JsonFieldPresence::Optional}},
                       {DATA_KEY_DATA,      {{js::obj_type}, JsonFieldPresence::Optional}},
					   {DATA_KEY_ChainId,   {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_NONCE,     {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_GAS,       {{js::str_type}, JsonFieldPresence::Required}},
                       {DATA_KEY_Price,     {{js::str_type}, JsonFieldPresence::Required}}}

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
	u256 chainId = -4;
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
        std::string send_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_getBalance\",\"params\":[\""+ toHex(add)  +"\", \"latest\"],\"id\":1}";
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
    _httpClient.SetTimeout(1500);
    _httpClient.SendRPCMessage(send_msg, _result);

    cwarn <<"\n"<< _result;

}

bool sign_trx_from_json(const bfs1::path &path, bool _is_send, std::string _ip = "") {
    std::pair<bool , std::string> _pair={false, ""};
    try {
        std::string hash_t = "";
        _pair = wallet::ToolTransaction::sign_trx_from_json(contentsString(path.string()), hash_t, _ip);

        if (_pair.first && _is_send) {
            sendRawTransation(_pair.second, _ip);
        }
    }
    catch (...){
        std::cout << "Error can not to send:"<<_ip<<std::endl;
        exit(1);
    }
    if(!_pair.first){
        std::cout <<"send field:\n"<< _pair.second <<std::endl;
        exit(1);
    } else{
        std::cout << "send ok " << "rlp :"<< _pair.second<<std::endl;
    }
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
    std::cout << "private-key: " << dev::crypto::to_base58((char*)sec.data(), 32) << std::endl;
    std::cout << "address    : " << key_pair.address() << std::endl;
    std::cout << "public-key : " << toString(key_pair.pub())<<std::endl;
    std::cout << "new-address: " << jsToNewAddress(key_pair.address()) << std::endl;

}

void format_private(const std::string &key){
    if(key.substr(0, 2) == "0x"){
        KeyPair pair(Secret(fromHex(key)));
        std::cout << "address : " << pair.address() << std::endl;
        std::cout << "pri-base58 : " << dev::crypto::to_base58((char*)pair.secret().data(), 32) << std::endl;
        std::cout << "pri-big int : " << toHex(pair.secret().ref()) << std::endl;
        std::cout << "public address : " << toHex(pair.pub()) << std::endl;
        std::cout << "new-address : " << jsToNewAddress(pair.address()) << std::endl;
    }
    else{
        auto pair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(key)));
        std::cout << "address : " << pair.address() << std::endl;
        std::cout << "pri-base58 : " << dev::crypto::to_base58((char*)pair.secret().data(), 32) << std::endl;
        std::cout << "pri-big int : " << toHex(pair.secret().ref()) << std::endl;
        std::cout << "public address : " << toHex(pair.pub()) << std::endl;
        std::cout << "new-address : " << jsToNewAddress(pair.address()) << std::endl;
    }
}

void getNewAddressFromOld(std::string const& _oldAddr){
    try{
        auto _addr = jsToAddress(_oldAddr);
        std::cout<<"newAddress: "<< jsToNewAddress(_addr) <<"\n\n";
    }
    catch (...){
        std::cout << "Error! the address:"<< _oldAddr << "  is error" <<"\n\n";
    }
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
                ("sha3", bpo::value<std::string>(), "caculate string sha3.")
                ("pri-tf", bpo::value<std::string>(), "private-key format.")
                ("newaddress", bpo::value<std::string>(), "to newAddress by old Address.")
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

        if(args_map.count("sha3")){
            auto ret = sha3(args_map["sha3"].as<std::string>());
            cwarn << ret;
            return  0;
        }

        if(args_map.count("pri-tf")){
            format_private(args_map["pri-tf"].as<std::string>());
            return  0;
        }

        if(args_map.count("newaddress")){
            auto oldAddr = args_map["newaddress"].as<std::string>();
            getNewAddressFromOld(oldAddr);
        }


    }catch (const std::exception &e){
        cwarn << e.what();
    }catch (const boost::exception &e){
        cwarn << boost::diagnostic_information(e);
    }catch (...){
        cwarn << "unkown exception ....";
    }




    return 0;
}
