#include <libdevcore/Common.h>
#include <libdevcrypto/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/vector_ref.h>
#include <libdevcrypto/base58.h>
#include <libdevcore/SHA3.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/Log.h>
#include <Json/Value.h>
#include <json_spirit/JsonSpiritHeaders.h>

#include <libbrcdchain/Transaction.h>
using namespace dev;
using namespace std;

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace js = json_spirit;


bool self_sign(const std::string &_from, const std::string &_to, uint32_t blockNumber, const std::string &privateKey, bool tofile){
    Address from(_from);
    Address to(_to);

    RLPStream _rlp(3);
    _rlp << from << to << blockNumber;
    auto _keypair = KeyPair(Secret(dev::crypto::from_base58(privateKey)));
    Signature  _sign = sign(_keypair.secret(), sha3(_rlp.out()));

    Json::Value vv;
    vv["from"] = toHex(from);
    vv["to"] = toHex(to);
    vv["blockNumber"] = blockNumber;
    vv["self-sign"] = toHex(_sign);
    if(!tofile){
        cwarn << "result : \n" << vv.toStyledString();
    }
    else{
        // TODO
    }

    return true;
}




bool miner_sign(const std::string &source_data, const std::string &privateKey, bool tofile){

    js::mValue val;
    js::read_string(source_data, val);
    js::mObject obj = val.get_obj();

    Address from = Address(obj["from"].get_str());
    Address to = Address(obj["to"].get_str());
    uint32_t blockNumber = obj["blockNumber"].get_int();
    Signature sign(fromHex(obj["self-sign"].get_str()));


    //verity sign
    RLPStream s(3);
    s << from << to << blockNumber;
    auto _hash = sha3(s.out());
    auto p = recover(sign, _hash);
    auto recover_address = right160(dev::sha3(bytesConstRef(p.data(), sizeof(p))));
    if(from != recover_address){
        cwarn << "check from sign error.";
        return false;
    }





//
//    auto _keypair = KeyPair(Secret(dev::crypto::from_base58(privateKey)));
//    Signature  _sign = sign(_keypair.secret(), sha3(_rlp.out()));
//
//    RLPStream _rlp(4);
//    _rlp << from << to << blockNumber;


    return true;
}



int main(int argc, char *argv[])
{
    try{
        bpo::options_description description("command line ");
        description.add_options()
                ("help,h", "show help message.")
                ("from,f", bpo::value<std::string>(), "Address to be modified")
                ("to,t", bpo::value<std::string>(), "Modified address")
                ("blockNumber,n", bpo::value<uint32_t>(), "Modify the certifier's block height")
                ("privateKey,k", bpo::value<std::string>(), "from address private key")
                ("consoleLog,l", bpo::value<bool>()->default_value(false),"Whether to write to the update_miner.json file in the current path")
                ("sourceData,d", bpo::value<std::string>(),"Data that needs to be signed by the current node")
                ("txJson,t", bpo::value<bfs::path>(), "Initiate a json file path to modify the verifier transaction")
                ;
        bpo::variables_map args_map;
        bpo::parsed_options parsed = bpo::parse_command_line(argc, argv, description);
        bpo::store(parsed, args_map);
        bool isWriteToFile = false;

        bool console =  args_map["consoleLog"].as<bool>();
        if (args_map.count("help")) {
            std::cout << description << std::endl;
            return 0;
        }

        // Generate signature data with the private key of "from"
        if (args_map.count("from") && args_map.count("to") && args_map.count("blockNumber") && args_map.count("privateKey")){
            auto ret = self_sign(args_map["from"].as<std::string>(),
                                 args_map["to"].as<std::string>(),
                                 args_map["blockNumber"].as<uint32_t>(),
                                 args_map["privateKey"].as<std::string>(),
                                 console );

            return 0;
        }else if(args_map.count("sourceData") && args_map.count("privateKey")){
            std::string source_data = args_map["sourceData"].as<std::string>();
            std::string private_key = args_map["privateKey"].as<std::string>();
            miner_sign(source_data, private_key, console);
            return 0;
        }
        else{
            cwarn << "please input [from, to, blockNumber, privateKey] or [sourceData, privateKey]";
        }


        // Sign the data signed by other nodes with your own private key

        // Send change miner transaction after getting the required signature data
    }catch (const std::exception &e){
        cwarn << e.what();
    }catch (const boost::exception &e){
        cwarn << boost::diagnostic_information(e);
    }catch (...){
        cwarn << "unkown exception ....";
    }
    return  0;
}