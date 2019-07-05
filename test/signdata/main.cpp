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
#include <json_spirit/JsonSpiritHeaders.h>
#include <json_spirit/json_spirit.h>


using namespace dev;
using namespace std;

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace js = json_spirit;

int main(int argc, char *argv[])
{
    try{
        bpo::options_description description("command line ");
        description.add_options()
                ("help,h", "show help message.")
                ("from,f", bpo::value<std::string>(), "Address to be modified")
                ("to,t", bpo::value<std::string>(), "Modified address")
                ("blockNumber,n", bpo::value<unsigned>(), "Modify the certifier's block height")
                ("privateKey,k", bpo::value<std::string>(), "from address private key")
                ("consoleLog,l", bpo::value<bool>(),"Whether to write to the update_miner.json file in the current path")
                ("sourceData,d", bpo::value<std::string>(),"Data that needs to be signed by the current node")
                ("txJson,t", bpo::value<bfs::path>(), "Initiate a json file path to modify the verifier transaction")
                ;
        bpo::variables_map args_map;
        bpo::parsed_options parsed = bpo::parse_command_line(argc, argv, description);
        bpo::store(parsed, args_map);
        bool isWriteToFile = false;

        if (args_map.count("help")) {
            std::cout << description << std::endl;
            return 0;
        }

        // Generate signature data with the private key of "from"
        if (args_map.count("from") && args_map.count("to") && args_map.count("blockNumber") && args_map.count("privateKey")){
            Address from(args_map["from"].as<std::string>());
            Address to(args_map["to"].as<std::string>());
            unsigned blockNumber = args_map["blockNumber"].as<unsigned>();
            auto privateKey(args_map["privateKey"].as<std::string>());

            RLPStream _rlp(3);
            _rlp << from << to << blockNumber;
            auto _keypair = KeyPair(Secret(dev::crypto::from_base58(privateKey)));
            Signature  _sign = sign(_keypair.secret(), sha3(_rlp.out()));

            js::mObject obj;
            obj["from"] = from.hex();
            //std::string res(js::write(obj));
            
            //std::cout << res;
            std::cout << from << " " << to << " " << blockNumber << std::endl;
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