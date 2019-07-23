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
#include <libdevcore/CommonJS.h>

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

//    std::string _key = "EgBYKnfwb4vzH5kwvdMqBCqUv7f16hz9f2eBXLCTCfRk";
//
//    auto _keypair = dev::crypto::from_base58(_key);
//
//    std::cout << _keypair << std::endl;



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

//    auto _key = KeyPair::create();
//    std::cout << _key.address() << std::endl;
//    std::cout << "private key: " << dev::crypto::to_base58((char*)_key.secret().data(), 32) << std::endl;

    std::string _key = "EgBYKnfwb4vzH5kwvdMqBCqUv7f16hz9f2eBXLCTCfRk";

    auto key = KeyPair(Secret(dev::crypto::from_base58(_key)));
    cnote << dev::crypto::from_base58(_key);
    cnote << key.pub();

    std::string _blockNum = "14";
    std::string blockHash = "0x0f4dcf08824f3ecb991704a07f63c7df0d9ca8c2e8e75fbd1de3fe2d08ae3f8e";
    std::string blockAuthor = "0xe523e7c59a0725afd08bc9751c89eed6f8e16dec";
    std::string nodeID = "b7a109792da7d053009fe5bdc7defba2a1d1b0e3170c077de8a16d3a77628b6a2addc5a00afc56f32512dbd21f98ee9c970cd344c24c00564579b52a456d0347";
    std::string time = "0x16bbb15f5c2";
    std::string clientVersion = "1c412a133f2be54dfed8e41ad79c2ef09e82d6cf";
    std::string nodeNum = "0x0";
    std::string nodeMaxDelay = "0x0";
    std::string nodeMinimumDelay  = "0x0";
    std::string packageTranscations = "0x0";
    std::string pendingpoolsnum = "0x0";


    RLPStream rlp(11);
    rlp << nodeID << _blockNum << blockAuthor<< blockHash << time << clientVersion << nodeNum << nodeMaxDelay << nodeMinimumDelay << packageTranscations << pendingpoolsnum;
    cnote << rlp.out();

    cnote  << toJS(sha3(rlp.out()));

    Signature  _sign = sign(key.secret(), sha3(rlp.out()));

    cnote << toJS(_sign);
}


//{
//"blockAuthor": "0xe523e7c59a0725afd08bc9751c89eed6f8e16dec",
//"blockHash": "0x0f4dcf08824f3ecb991704a07f63c7df0d9ca8c2e8e75fbd1de3fe2d08ae3f8e",
//"blockNum": 14,
//"clientVersion": "1c412a133f2be54dfed8e41ad79c2ef09e82d6cf",
//"nodeID": "b7a109792da7d053009fe5bdc7defba2a1d1b0e3170c077de8a16d3a77628b6a2addc5a00afc56f32512dbd21f98ee9c970cd344c24c00564579b52a456d0347",
//"nodeMaxDelay": "0x0",
//"nodeMinimumDelay": "0x0",
//"nodeNum": "0x0",
//"packageTranscations": "0x0",
//"pendingpoolsnum": "0x0",
//"serverDelay": "0x16bbb15f5c2",
//"signatureData": "0x99ad654e7bdf77fefc16aa609252840f77111b2637e93ae604abd21ce9262ac6413e1b8ca289b791ab4d478bd78ef0861142e541a882bff4345e349cce6c106101"
//}
