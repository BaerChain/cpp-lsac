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
                cwarn << "test transfer_to_addresss result: " << result;
            }


        }catch (const std::exception &e){
            std::cout << "transfer_to_addresss " << e.what() << std::endl;
        }catch (...){
            std::cout << "transfer_to_addresss \n";
        }


    }

}