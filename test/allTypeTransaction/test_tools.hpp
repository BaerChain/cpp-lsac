//
// Created by friday on 22/05/2019.
//
#pragma once

#include <string>
#include <libbrcdchain/Transaction.h>
#include <libdevcore/Address.h>
#include "helper.hpp"

namespace test_tools{


    class test_tools{
    public:
        test_tools(const std::string &seed_private, const std::string &ip_port);

        void run();
    private:

        void init_key();


        void transfer_to_addresss();

        void convert_brc_to_cook();

        void create_contract_and_call();

        account get_account(const Address &add);

        template <typename T>
        T   get_value_from_rpc(const std::string &s);


        std::string sendRawTransation(const std::string &data);


        bytes packed_operation_data(const std::vector<std::shared_ptr<dev::brc::transationTool::operation>> &op);

        std::string _source_private;
        dev::Address _source_address;
        dev::Secret     _secret_key;
        std::string _ip_port;
        std::map<Address, Secret>   _address_keys;
    };
}
