#include <iostream>

#include <libdevcore/CommonIO.h>
#include <libdevcore/FileSystem.h>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>

#include <libp2p/Host.h>

#include <libdevcrypto/Common.h>
#include <libdevcrypto/base58.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace fs = boost::filesystem;
namespace bop = boost::program_options;
using namespace dev;

KeyPair networkAlias(bytesConstRef _b)
{
    RLP r(_b);
    if (r.itemCount() == 3 && r[0].isInt() && r[0].toInt<unsigned>() >= 3)
        return KeyPair(Secret(r[1].toBytes()));
    else
        return KeyPair::create();
}



int main(int argc, char *argv[])
{
    // bop::options_description _description("command line ");
    // _description.add_options()
    //     ("data,d", bop::value<std::string>(), "data path");
    
    // bop::variables_map args_map;
    // bop::parsed_options parsed = bop::parse_command_line(argc, argv, _description);
    // bop::store(parsed, args_map);
    // if(args_map.count("data"))
    // {
    //     setDataDir(args_map["data"].as<std::string>());
    // }


    // auto netStates = contents(getDataDir() / fs::path("network.rlp"));

    // KeyPair _key =  networkAlias(&netStates);

    // std::cout << " public_key: " << _key.pub() << std::endl;
    // std::cout << " address :" << _key.address() << std::endl;
    // std::cout << " private_key: " << crypto::to_base58((char *)_key.secret().data(),32) << std::endl;


    // std::string _nodeid = "6c3f9db0041317229d4d517bcdd34b3517002a323e1a3a3b7483985c6f7702ad4e2fe8e3c848adaacb9f11d62b42646aafef654833519e01bf714debda27368e";
    // std::string _blockNum = "0x1";
    // std::string _blockHash = "0x48f683f4b75fca76a9c6a31c2a04fc3e1801d2078bd87e0f4234aacb620a8244";
    // std::string _version = "9d4ba962fb6addf7e02a15a57208138d0ac99ff5";
    // std::string _serverDelay = "40";
    // std::string _packagedTransactions = "0x100";
    // std::string _tradingpoolsNum = "0x100";

    // std::vector<std::string> _blockDelay = { "40", "200", "50"};
    // std::vector<std::string> _nodeinformation = { "0x32", "40", "200"};


    // RLPStream _rlp;
    // _rlp.appendList(9);
    // _rlp << _nodeid << _blockNum << _blockHash << _version << _serverDelay; 

    // {
    //     RLPStream _blockrlp;
    //     size_t _blockDelayNum = _blockDelay.size();
    //     _blockrlp.appendList(_blockDelayNum +1);
    //     _blockrlp << _blockDelayNum;
    //     for(auto it : _blockDelay)
    //     {
    //         _blockrlp << it;
    //     }

    //     _rlp << _blockrlp.out();
    // }

    // {
    //     RLPStream _noderlp;
    //     size_t _nodeNum = _nodeinformation.size();
    //     _noderlp.appendList(_nodeNum+1);
    //     _noderlp << _nodeNum;
    //     for(auto it : _nodeinformation)
    //     {
    //         _noderlp << it;
    //     }

    //     _rlp << _noderlp.out();
    // }
 
    // _rlp << _packagedTransactions << _tradingpoolsNum;
    
    
    // std::cout << "_rlp:" << _rlp.out() << std::endl;

    // Signature _signature = dev::sign(_key.secret(), sha3(_rlp.out()));

    // std::cout << "signature :" <<_signature << std::endl;

    // if(dev::verify(_key.pub(), _signature, sha3(_rlp.out())))
    // {
    //     std::cout << "verify success" << std::endl;
    // }else{
    //     std::cout << "verify error" << std::endl;
    // }

//    std::string _from = "0xe523e7c59a0725afd08bc9751c89eed6f8e16dec";
//    std::string _to = "0x042610e447c94ff0824b7aa89fab123930edc805";
//    std::string _key = "8RioSGhgNUKFZopC2rR3HRDD78Sc48gci4pkVhsduZve";

    //std::string _key = "4DS7PYhDYQWToua45gFnHG2GXCSVuVwSMx2ZyUeFxhqb";

    //auto _keypair = dev::crypto::from_base58(_key);

    //std::cout << _keypair << std::endl;



//    unsigned _blockNum = 20;
//
//    RLPStream _rlp(3);
//    _rlp << Address(_from) << Address(_to) << _blockNum;
//
//    // Secret _ret(_key);
//    auto keyPair = KeyPair(Secret(dev::crypto::from_base58(_key)));
//
//    Signature _sign = dev::sign(keyPair.secret(), sha3(_rlp.out()));
//
//    std::cout << "sign:" <<_sign << std::endl;

    auto _key = KeyPair::create();
    std::cout << _key.address() << std::endl;
    std::cout << "private key: " << dev::crypto::to_base58((char*)_key.secret().data(), 32) << std::endl;

}
