
#include <libdevcore/SHA3.h>
#include <libdevcore/Log.h>
#include <iostream>
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


using namespace dev::brc;
using namespace dev;


void create_contract(){
    std::string test = "multiply(uint256)";
    auto rr = dev::sha3(test);
    std::cout << rr ;

    std::string contract_data = "0x608060405234801561001057600080fd5b506101f1806100206000396000f3"
                                "0060806040526004361061004c576000357c0100000000000000000000000000"
                                "000000000000000000000000000000900463ffffffff16806311114af1146100"
                                "51578063c6888fa114610133575b600080fd5b34801561005d57600080fd5b50"
                                "6100b8600480360381019080803590602001908201803590602001908080601f"
                                "0160208091040260200160405190810160405280939291908181526020018383"
                                "808284378201915050505050509192919290505050610174565b604051808060"
                                "2001828103825283818151815260200191508051906020019080838360005b83"
                                "8110156100f85780820151818401526020810190506100dd565b505050509050"
                                "90810190601f1680156101255780820380516001836020036101000a03191681"
                                "5260200191505b509250505060405180910390f35b34801561013f57600080fd"
                                "5b5061015e6004803603810190808035906020019092919050505061017e565b"
                                "6040518082815260200191505060405180910390f35b6060819050919050565b"
                                "60007f24abdb5865df5079dcc5ac590ff6f01d5c16edbc5fab4e195d9febd111"
                                "4503da600783026040518082815260200191505060405180910390a160078202"
                                "90509190505600a165627a7a72305820b7dcf3ffc23d38b70328fb0686936139"
                                "65fa00a36d684661ae2f53df694f1e570029";
    std::vector<byte> vec_data;
    for(auto c : contract_data){
        vec_data.push_back((byte)c);
    }

    auto key = "8RioSGhgNUKFZopC2rR3HRDD78Sc48gci4pkVhsduZve";
    auto keyPair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(key)));
    auto address = keyPair.address();

    dev::brc::TransactionSkeleton ts;
    ts.creation = true;
    ts.from = address;
    ts.data = vec_data;
    ts.nonce = u256(0);
    ts.gas = u256(0x63880);
    ts.gasPrice = u256(0x13880);

    dev::brc::Transaction sign_t(ts, keyPair.secret());

    auto sssss = dev::brc::toJson(sign_t);
    cerror << "test: " << sssss << std::endl;
    cerror << "test: " << toHexPrefixed(sign_t.rlp()) << std::endl;

}


int main(){
    create_contract();



    return 0;
}