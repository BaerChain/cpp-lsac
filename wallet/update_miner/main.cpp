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
#include <libweb3jsonrpc/JsonHelper.h>
#include <libbrcdchain/Transaction.h>
#include <libdevcore/CommonIO.h>

using namespace dev;
using namespace std;

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace js = json_spirit;
namespace dbt = dev::brc::transationTool;


bool self_sign(const std::string &_from, const std::string &_to, uint32_t blockNumber, const std::string &privateKey,
               const std::string & tofile) {
    Address from(_from);
    Address to(_to);

    RLPStream _rlp(3);
    _rlp << from << to << blockNumber;
    auto _keypair = KeyPair(Secret(dev::crypto::from_base58(privateKey)));
    Signature _sign = sign(_keypair.secret(), sha3(_rlp.out()));

    Json::Value vv;
    vv["from"] = toHex(from);
    vv["to"] = toHex(to);
    vv["blockNumber"] = blockNumber;
    vv["self-sign"] = toHex(_sign);
    vv["other-signs"] = Json::Value(Json::ValueType::arrayValue);
    if (!tofile.size()) {
        cwarn << "result : \n" << vv.toStyledString();
    } else {
        cwarn << "complete! write to file " << tofile;
        dev::writeFile(tofile, vv.toStyledString());
    }

    return true;
}


bool miner_sign(const std::string &source_data, const std::string &privateKey, const std::string &tofile) {

    js::mValue val;
    js::read_string(source_data, val);
    js::mObject obj = val.get_obj();

    Address from = Address(obj["from"].get_str());
    Address to = Address(obj["to"].get_str());
    uint32_t blockNumber = obj["blockNumber"].get_int();
    Signature source_sign(fromHex(obj["self-sign"].get_str()));
    auto others_sign = obj["other-signs"].get_array();

    //verity sign
    {
        RLPStream s(3);
        s << from << to << blockNumber;
        auto _hash = sha3(s.out());
        auto p = recover(source_sign, _hash);
        auto recover_address = right160(dev::sha3(bytesConstRef(p.data(), sizeof(p))));
        if (from != recover_address) {
            cwarn << "check from(address) !=  sign error.";
            return false;
        } else {
            cwarn << "check data sign success.";
        }
    }


    RLPStream data(4);
    data << from << to << blockNumber << source_sign;

    auto _keypair = KeyPair(Secret(dev::crypto::from_base58(privateKey)));
    Signature other_sign = sign(_keypair.secret(), sha3(data.out()));
    Json::Value vv;
    vv["from"] = toHex(from);
    vv["to"] = toHex(to);
    vv["blockNumber"] = blockNumber;
    vv["self-sign"] = toHex(source_sign);


    auto value = Json::Value(Json::ValueType::arrayValue);
    value.append(toHex(other_sign));

    for(auto itr : others_sign){
        value.append(Json::Value(itr.get_value<std::string>()));
    }

    vv["other-signs"] = value;

    if (!tofile.size()) {
        cwarn << "result : \n" << vv.toStyledString();
    } else {
        cwarn << "complete! write to file " << tofile;
        dev::writeFile(tofile, vv.toStyledString());
    }


    return true;
}


bool sign_update_miner_tx(const std::string &content, const std::string &private_key, const std::string &server) {
    js::mValue val;
    js::read_string(content, val);
    js::mObject obj = val.get_obj();

    auto _keypair = KeyPair(Secret(dev::crypto::from_base58(private_key)));


    Address from = Address(obj["from"].get_str());
    Address to = Address(obj["to"].get_str());
    uint32_t blockNumber = obj["blockNumber"].get_int();
    Signature source_sign(fromHex(obj["self-sign"].get_str()));
    auto others_sign = obj["other-signs"].get_array();

    std::vector<Signature> m_agreeMsgs;
    for(auto itr : others_sign){
        auto str = itr.get_value<std::string>();
        m_agreeMsgs.push_back(Signature(fromHex(str)));
    }

    dbt::changeMiner_operation op(dbt::op_type::changeMiner, from, to, blockNumber, source_sign, m_agreeMsgs);

    u256 nonce;
    u256 gas;
    u256 gasPrice;

    if(obj.count("nonce")){
        nonce = u256(obj["nonce"].get_str());
    }
    else{
        cerror << "not find key nonce : " << content;
    }

    if(obj.count("gas")){
        gas = u256(obj["gas"].get_str());
    }
    else{
        cerror << "not find key gas : " << content;
    }

    if(obj.count("gasPrice")){
        gasPrice = u256(obj["gasPrice"].get_str());
    }
    else{
        cerror << "not find key gasPrice : " << content;
    }

    RLPStream rlp;
    std::vector<bytes> _v;
    _v.push_back(op.serialize());
    rlp.appendVector<bytes>(_v);

    brc::TransactionSkeleton ts;
    ts.from = from;
    ts.to = VoteAddress;
    ts.nonce = nonce;
    ts.gas = gas;
    ts.gasPrice = gasPrice;
    ts.data = rlp.out();
    ts.chainId = 0;

    brc::Transaction sign_t(ts, _keypair.secret());

    cwarn << "result: \n" << dev::brc::toJson(sign_t);

    cwarn << "rlp data : " << toHexPrefixed(sign_t.rlp());

    return true;
}


int main(int argc, char *argv[]) {
    try {
        bpo::options_description description("command line ");
        description.add_options()
                ("help,h", "show help message.")
                ("from,f", bpo::value<std::string>(), "Address to be modified")
                ("to,t", bpo::value<std::string>(), "Modified address")
                ("blockNumber,n", bpo::value<uint32_t>(), "Modify the certifier's block height")
                ("privateKey,k", bpo::value<std::string>(), "from address private key")
                ("write-file", bpo::value<std::string>(), "if set write-file, result will update file.")
                ("sourceData,d", bpo::value<std::string>(), "Data that needs to be signed by the current node")
                ("txJson,t", bpo::value<std::string>(), "Initiate a json file path to modify the verifier transaction")
                ("sign-tx", bpo::value<std::string>(), "package miners sign to transaction data.")
                ("server-ip", bpo::value<std::string>(), "node server ip, example: 127.0.0.1:8081, if set this value,  transaction will send to node.")
                ;
        bpo::variables_map args_map;
        bpo::parsed_options parsed = bpo::parse_command_line(argc, argv, description);
        bpo::store(parsed, args_map);

        if (args_map.count("help")) {
            std::cout << description << std::endl;
            return 0;
        }

        // Generate signature data with the private key of "from"
        if (args_map.count("from") && args_map.count("to") && args_map.count("blockNumber") &&
            args_map.count("privateKey")) {
            auto console = args_map["write-file"].as<std::string>();
            auto ret = self_sign(args_map["from"].as<std::string>(),
                                 args_map["to"].as<std::string>(),
                                 args_map["blockNumber"].as<uint32_t>(),
                                 args_map["privateKey"].as<std::string>(),
                                 console);

            return 0;
        } else if (args_map.count("sourceData") && args_map.count("privateKey")) {
            auto console = args_map["write-file"].as<std::string>();
            std::string source_data = args_map["sourceData"].as<std::string>();
            std::string private_key = args_map["privateKey"].as<std::string>();
            miner_sign(source_data, private_key, console);
            return 0;
        } else if (args_map.count("txJson") && args_map.count("privateKey")) {
            auto console = args_map["write-file"].as<std::string>();
            std::string private_key = args_map["privateKey"].as<std::string>();
            std::string json_file_path = args_map["txJson"].as<std::string>();
            std::string content_str;
            miner_sign(contentsString(json_file_path), private_key, console);
            return 0;
        } else if(args_map.count("sign-tx") && args_map.count("privateKey")){
            std::string server = args_map.count("server-ip") ? args_map["server-ip"].as<std::string>() : std::string();
            std::string private_key = args_map["privateKey"].as<std::string>();
            std::string json_file_path = args_map["sign-tx"].as<std::string>();
            sign_update_miner_tx(contentsString(json_file_path), private_key, server);
            return 0;
        }
        else {
            cerror
                << "please input params: [from, to, blockNumber, privateKey] , [sourceData, privateKey] ,[txJson, privateKey], [sign-tx, privateKey]";
        }


        // Sign the data signed by other nodes with your own private key

        // Send change miner transaction after getting the required signature data
    } catch (const std::exception &e) {
        cwarn << e.what();
    } catch (const boost::exception &e) {
        cwarn << boost::diagnostic_information(e);
    } catch (...) {
        cwarn << "unkown exception ....";
    }
    return 0;
}