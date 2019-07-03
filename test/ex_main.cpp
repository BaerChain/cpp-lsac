
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
#include <libbrcdchain/GasPricer.h>

using namespace dev::brc;
using namespace dev;


void create_contract(){
    std::string test = "multiply(uint256)";
    auto rr = dev::sha3(test);
    std::cout << rr ;

//    std::string contract_data = "0x608060405234801561001057600080fd5b506101f1806100206000396000f3"
//                                "0060806040526004361061004c576000357c0100000000000000000000000000"
//                                "000000000000000000000000000000900463ffffffff16806311114af1146100"
//                                "51578063c6888fa114610133575b600080fd5b34801561005d57600080fd5b50"
//                                "6100b8600480360381019080803590602001908201803590602001908080601f"
//                                "0160208091040260200160405190810160405280939291908181526020018383"
//                                "808284378201915050505050509192919290505050610174565b604051808060"
//                                "2001828103825283818151815260200191508051906020019080838360005b83"
//                                "8110156100f85780820151818401526020810190506100dd565b505050509050"
//                                "90810190601f1680156101255780820380516001836020036101000a03191681"
//                                "5260200191505b509250505060405180910390f35b34801561013f57600080fd"
//                                "5b5061015e6004803603810190808035906020019092919050505061017e565b"
//                                "6040518082815260200191505060405180910390f35b6060819050919050565b"
//                                "60007f24abdb5865df5079dcc5ac590ff6f01d5c16edbc5fab4e195d9febd111"
//                                "4503da600783026040518082815260200191505060405180910390a160078202"
//                                "90509190505600a165627a7a72305820b7dcf3ffc23d38b70328fb0686936139"
//                                "65fa00a36d684661ae2f53df694f1e570029";
    std::string contract_data = "6080604052336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055506102b7806100536000396000f300608060405260043610610057576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680631ab5d260146100595780638da5cb5b1461007b578063a8d0e679146100d2575b005b610061610129565b604051808215151515815260200191505060405180910390f35b34801561008757600080fd5b50610090610245565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b3480156100de57600080fd5b50610113600480360381019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919050505061026a565b6040518082815260200191505060405180910390f35b6000600a3073ffffffffffffffffffffffffffffffffffffffff163110151561023d573373ffffffffffffffffffffffffffffffffffffffff166108fc600a9081150290604051600060405180830381858888f19350505050158015610193573d6000803e3d6000fd5b507fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef3033600a604051808473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020018373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001828152602001935050505060405180910390a160019050610242565b600090505b90565b6000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60008173ffffffffffffffffffffffffffffffffffffffff163190509190505600a165627a7a72305820df3521e3f831c2bb5f6e302d5fe72f022277e08f706f1b1f03c404ff942775580029";
    std::vector<byte> vec_data;
    for(auto c : contract_data){
        vec_data.push_back((byte)c);
    }

    auto key = "6vsDoEvshzWJUhY5Aa8xd9WtHwNs9tjmeFq8NnJBST6t";
    auto keyPair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(key)));
    auto address = keyPair.address();

    dev::brc::TransactionSkeleton ts;
    ts.creation = true;
    ts.from = address;
    ts.data = vec_data;
    ts.nonce = u256(2);
    ts.gas = u256(0x138800);
    ts.gasPrice = u256(0x1388);

    dev::brc::Transaction sign_t(ts, keyPair.secret());

    auto sssss = dev::brc::toJson(sign_t);
    cerror << "test: " << sssss << std::endl;
    cerror << "test: " << toHexPrefixed(sign_t.rlp()) << std::endl;

}


int main(){

    RLPStream rlp;
    std::vector<uint8_t >  cc;
    cc.push_back(4);

    rlp.appendVector(cc);

    std::cout << toJS(rlp.out()) << std::endl;






//    create_contract();


    return 0;
}