//
// Created by friday on 22/05/2019.
//


#include "test_tools.hpp"
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
using namespace dev;
using namespace dev::brc::transationTool;

namespace test_tools{


    test_tools::test_tools(const std::string &seed_private, const std::string &ip_port) {
        _source_private = seed_private;
        _ip_port = ip_port;


    }

    void test_tools::run() {
        //init keys.
        init_key();

        //create contract.
        create_contract_and_call();
        sleep(2);
        //transfer address by source address.
        transfer_to_addresss();
        sleep(2);


        convert_brc_to_cook();



    }

    void test_tools::init_key() {
        std::vector<std::string>   base58_key = {
                "4tAUjwA4guRhYArjiJ9JVf7QTmNLjinc3UfChccJF6tC",
                "GkX7iw1z7D6S66FH1PekmqjHEVg5zYAXyxycCpnT58Sd",
                "J3PJ6kDuwU6k3ScaELmnZKyc1Njjz8auYbngohx5turb",
                "8gVai1wyj9XTrNjXUmafbb1iGCQ68YEydgZaPyK45TDE",
                "HPhkJBdy6TDScmtjTczctwSkMMF4VFQyYjhGiA8tMXke",
                "86w6mfpZWh7qVP6LgQXtp2GGyjUWvn6FWjH3JPTNCx1V",
                "EiMRoHzUxMC3SdoDwXnAsVmFsaPKboYRCdJ6H691yPhs",
                "CidvVc6tDfxmDM9VeF9z5cpztMvcrK4UDQfFn8jb2AhQ"
        };

        for(auto key : base58_key){
            auto keyPair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(key)));
            _address_keys[keyPair.address()] = keyPair.secret();
        }

        {
            auto keyPair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(_source_private)));
            _source_address = keyPair.address();
            _secret_key = keyPair.secret();
        }

    }

    void test_tools::transfer_to_addresss() {
        try {
            u256 transfer_value = u256(0xfffffff);
            std::vector<std::shared_ptr<operation>> ops;
            for(auto itr : _address_keys){
                auto op  = new transcation_operation(op_type::brcTranscation, _source_address, itr.first, 1, transfer_value);
                ops.push_back(std::shared_ptr<transcation_operation>(op));
            }

            brc::TransactionSkeleton ts;
            ts.data = packed_operation_data(ops);
            ts.from = _source_address;
            ts.to = Address("0x00000000000000000000000000000000766f7465");
            ts.value = 0;
            ts.gas = string_to_u256("0xfffffffffffff");
            ts.gasPrice = 0x1388;
            ts.nonce = get_account(_source_address).nonce;

            brc::Transaction sign_t(ts, _secret_key);
            auto rlp_data = dev::brc::toJson(sign_t);
//            cwarn << "send data : " << rlp_data;

            auto result = sendRawTransation(toHexPrefixed(sign_t.rlp()));
            cwarn << "test transfer_to_addresss result: " << result;
        }catch (const std::exception &e){
            std::cout << "transfer_to_addresss " << e.what() << std::endl;
        }catch (...){
            std::cout << "transfer_to_addresss \n";
        }



    }

    account test_tools::get_account(const Address &add) {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_getBalance\",\"params\":[\""+ toHex(add) +"\", \"-1\"],\"id\":1}";
        std::string response =   post_message(request, _ip_port);;
        return get_value_from_rpc<account>(response);
    }

    template<typename T>
    T test_tools::get_value_from_rpc(const std::string &rpc_str) {
        request_rpc rpc;
        to_variant(string_to_json(rpc_str), rpc);
        T t;
        to_variant<T>(rpc.result, t);

        return t;
    }

    std::string test_tools::sendRawTransation(const std::string &data) {
        std::string send_msg = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_sendRawTransaction\",\"params\":[\"" + data +"\"],\"id\":1}";
//        cwarn << send_msg;
        return post_message(send_msg, _ip_port);
    }


    bytes test_tools::packed_operation_data(const std::vector<std::shared_ptr<operation>> &op) {
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

    void test_tools::convert_brc_to_cook() {

        try {

            for(auto itr : _address_keys){

                u256 transfer_value = u256(0xfffffff) / 2;
                std::vector<std::shared_ptr<operation>> ops;


                auto op  = new pendingorder_opearaion(op_type::pendingOrder,
                        itr.first,
                        dev::brc::ex::order_type::buy,
                        dev::brc::ex::order_token_type::BRC,
                        dev::brc::ex::order_buy_type::all_price,
                                                      0,
                                                      transfer_value
                                        );
                ops.push_back(std::shared_ptr<pendingorder_opearaion>(op));

                brc::TransactionSkeleton ts;
                ts.data = packed_operation_data(ops);
                ts.from = itr.first;
                ts.to = Address("0x00000000000000000000000000000000766f7465");
                ts.value = 0;
                ts.gas = string_to_u256("0xfffff");
                ts.gasPrice = 0x1388;
                ts.nonce = get_account(itr.first).nonce;

                brc::Transaction sign_t(ts, itr.second);
//                auto rlp_data = dev::brc::toJson(sign_t);
//                cwarn << "send data : " << rlp_data;
                ops.clear();
                auto result = sendRawTransation(toHexPrefixed(sign_t.rlp()));
                cwarn << "test pending order tx result: " << result;
            }


        }catch (const std::exception &e){
            std::cout << "transfer_to_addresss " << e.what() << std::endl;
        }catch (...){
            std::cout << "transfer_to_addresss \n";
        }


    }

    void test_tools::create_contract_and_call() {
        std::string code = "0x608060405241600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055504460025545600355436004555a600555346006553a60075532600860006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055503480156100aa57600080fd5b50336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555061086a806100fa6000396000f3006080604052600436106100af576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806342cbb15c146100b4578063455259cb146100df57806351be4eaa1461010a578063893d20e8146101355780638da5cb5b1461018c578063a16963b3146101e3578063a17042cc1461020e578063a7993bc714610239578063c8e7ca2e14610264578063d1a82a9d146102f4578063df1f29ee1461034b575b600080fd5b3480156100c057600080fd5b506100c96103a2565b6040518082815260200191505060405180910390f35b3480156100eb57600080fd5b506100f46103e5565b6040518082815260200191505060405180910390f35b34801561011657600080fd5b5061011f610428565b6040518082815260200191505060405180910390f35b34801561014157600080fd5b5061014a61046b565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34801561019857600080fd5b506101a1610518565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b3480156101ef57600080fd5b506101f861053d565b6040518082815260200191505060405180910390f35b34801561021a57600080fd5b50610223610580565b6040518082815260200191505060405180910390f35b34801561024557600080fd5b5061024e6105c3565b6040518082815260200191505060405180910390f35b34801561027057600080fd5b50610279610606565b6040518080602001828103825283818151815260200191508051906020019080838360005b838110156102b957808201518184015260208101905061029e565b50505050905090810190601f1680156102e65780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b34801561030057600080fd5b506103096106e0565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34801561035757600080fd5b5061036061078f565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b60007fc2465e6b0f9756e282efb59953193d8c737a188c7d4f6eb9185913f7b1d971956004546040518082815260200191505060405180910390a1600454905090565b60007f4a62132d30272ccb87702c32f74249a7300a66e713bbcac8dcc08d7f09ea50906007546040518082815260200191505060405180910390a1600754905090565b60007ff1ffae2c67ff490c8405617ea574c30eb50e71c9a7358ac9597a8c5d6c8ca4a16005546040518082815260200191505060405180910390a1600554905090565b60007fe3dc01f26841462f7605d43d79c5b39ef562d9d9726f2a390f728966016ae6456000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff16604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390a16000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b6000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60007f9e76974886f2eda5c265f310d9ac4e98d696f0f9e373345fd6c0c739c69c971c6003546040518082815260200191505060405180910390a1600354905090565b60007f3f0bfaf15ce87d7650278324bfa256b6b11448db7c6cc1aeeecef77212d07b876006546040518082815260200191505060405180910390a1600654905090565b60007f1480dc93812a08d7c5e97a046f2f9e559346c636dc7d173eb05fcb0dcdec40fc6002546040518082815260200191505060405180910390a1600254905090565b60606000368080601f01602080910402602001604051908101604052809392919081815260200183838082843782019150505050505090507fd08f89a944802d7b1be1d97d79c240642c004f7370b37196c942660c05c7e5d9816040518080602001828103825283818151815260200191508051906020019080838360005b838110156106a0578082015181840152602081019050610685565b50505050905090810190601f1680156106cd5780820380516001836020036101000a031916815260200191505b509250505060405180910390a180905090565b60007f812c77ce7adf6df59d6798fe71cbac60ceffd1c7e6d10cc77fb5272079131a40600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390a1600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b60007fc4401aedacccee8862acfd1ce187ea1308546452605b7a3d9bb874ee114d0a84600860009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390a1600860009054906101000a900473ffffffffffffffffffffffffffffffffffffffff169050905600a165627a7a7230582011ed94e88e1da72e0ef4c7af899392c9db7d2070ec881414999857f4ab62250b0029";

        brc::TransactionSkeleton ts;
        ts.data = fromHex(code);
        ts.from = _source_address;
        ts.value = 10000;
        ts.gas = string_to_u256("0xffffffffffff");
        ts.gasPrice = 0x1388;
        ts.nonce = get_account(_source_address).nonce;

        brc::Transaction sign_t(ts, _secret_key);
        auto result = sendRawTransation(toHexPrefixed(sign_t.rlp()));
        cwarn << "test create contrct tx result: " << result;

    }
}