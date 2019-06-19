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


u256 transfer_value = u256(0xffffffffff);



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

        //create contract.
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

                u256 transfer_value_s = transfer_value / 2;
                std::vector<std::shared_ptr<operation>> ops;


                auto op  = new pendingorder_opearaion(op_type::pendingOrder,
                        itr.first,
                        dev::brc::ex::order_type::buy,
                        dev::brc::ex::order_token_type::BRC,
                        dev::brc::ex::order_buy_type::all_price,
                                                      0,
                                                      transfer_value_s
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
        std::string code = "0x608060405241600260006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055504460035545600455436005556000366006919061006392919061010b565b505a600755346008553a60095532600a60006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555042600b5542600c553480156100c557600080fd5b50336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055506101b0565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f1061014c57803560ff191683800117855561017a565b8280016001018555821561017a579182015b8281111561017957823582559160200191906001019061015e565b5b509050610187919061018b565b5090565b6101ad91905b808211156101a9576000816000905550600101610191565b5090565b90565b610ae3806101bf6000396000f3006080604052600436106100d0576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806342cbb15c146100d5578063455259cb1461010057806351be4eaa1461012b578063893d20e8146101565780638da5cb5b146101ad578063a16963b314610204578063a17042cc1461022f578063a7993bc71461025a578063bbe4fd5014610285578063c8e7ca2e146102b0578063d1a82a9d14610340578063df1f29ee14610397578063e27a9563146103ee578063ee82ac5e14610419575b600080fd5b3480156100e157600080fd5b506100ea6104bf565b6040518082815260200191505060405180910390f35b34801561010c57600080fd5b50610115610502565b6040518082815260200191505060405180910390f35b34801561013757600080fd5b50610140610545565b6040518082815260200191505060405180910390f35b34801561016257600080fd5b5061016b610588565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b3480156101b957600080fd5b506101c2610635565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34801561021057600080fd5b5061021961065a565b6040518082815260200191505060405180910390f35b34801561023b57600080fd5b5061024461069d565b6040518082815260200191505060405180910390f35b34801561026657600080fd5b5061026f6106e0565b6040518082815260200191505060405180910390f35b34801561029157600080fd5b5061029a610723565b6040518082815260200191505060405180910390f35b3480156102bc57600080fd5b506102c5610766565b6040518080602001828103825283818151815260200191508051906020019080838360005b838110156103055780820151818401526020810190506102ea565b50505050905090810190601f1680156103325780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b34801561034c57600080fd5b506103556108c2565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b3480156103a357600080fd5b506103ac610971565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b3480156103fa57600080fd5b50610403610a20565b6040518082815260200191505060405180910390f35b34801561042557600080fd5b5061044460048036038101908080359060200190929190505050610a63565b6040518080602001828103825283818151815260200191508051906020019080838360005b83811015610484578082015181840152602081019050610469565b50505050905090810190601f1680156104b15780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b60007fc2465e6b0f9756e282efb59953193d8c737a188c7d4f6eb9185913f7b1d971956005546040518082815260200191505060405180910390a1600554905090565b60007f4a62132d30272ccb87702c32f74249a7300a66e713bbcac8dcc08d7f09ea50906009546040518082815260200191505060405180910390a1600954905090565b60007ff1ffae2c67ff490c8405617ea574c30eb50e71c9a7358ac9597a8c5d6c8ca4a16007546040518082815260200191505060405180910390a1600754905090565b60007fe3dc01f26841462f7605d43d79c5b39ef562d9d9726f2a390f728966016ae6456000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff16604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390a16000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b6000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60007f9e76974886f2eda5c265f310d9ac4e98d696f0f9e373345fd6c0c739c69c971c6004546040518082815260200191505060405180910390a1600454905090565b60007f3f0bfaf15ce87d7650278324bfa256b6b11448db7c6cc1aeeecef77212d07b876008546040518082815260200191505060405180910390a1600854905090565b60007f1480dc93812a08d7c5e97a046f2f9e559346c636dc7d173eb05fcb0dcdec40fc6003546040518082815260200191505060405180910390a1600354905090565b60007f075540403a53d31b9b33be9ad5a3e14adb21dbaaa3f8fde76cb7451ce8eb4da6600b546040518082815260200191505060405180910390a1600b54905090565b60607fd08f89a944802d7b1be1d97d79c240642c004f7370b37196c942660c05c7e5d9600660405180806020018281038252838181546001816001161561010002031660029004815260200191508054600181600116156101000203166002900480156108145780601f106107e957610100808354040283529160200191610814565b820191906000526020600020905b8154815290600101906020018083116107f757829003601f168201915b50509250505060405180910390a160068054600181600116156101000203166002900480601f0160208091040260200160405190810160405280929190818152602001828054600181600116156101000203166002900480156108b85780601f1061088d576101008083540402835291602001916108b8565b820191906000526020600020905b81548152906001019060200180831161089b57829003601f168201915b5050505050905090565b60007f812c77ce7adf6df59d6798fe71cbac60ceffd1c7e6d10cc77fb5272079131a40600260009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390a1600260009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b60007fc4401aedacccee8862acfd1ce187ea1308546452605b7a3d9bb874ee114d0a84600a60009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390a1600a60009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b60007faf20931cdb52eacbc5a87818b1185b70c77e7019cf082148abac911c4494bba6600c546040518082815260200191505060405180910390a1600c54905090565b60608140600181600019169055507fabc0ea39d08640a1973cd6babee96c1b440e1a595e846ef82ad3bd0f5c3e6fde60015460405180826000191660001916815260200191505060405180910390a19190505600a165627a7a723058202d81cc5b440fe59e6d9628c05ab14266f4acac76aeba1ee24332749c1c044c070029";

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

    void test_tools::send_orders() {





    }
}