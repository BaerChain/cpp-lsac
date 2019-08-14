//
// Created by lzb on 19-8-10.
//

#ifndef BAERCHAIN_TOOLTRANSACTION_H
#define BAERCHAIN_TOOLTRANSACTION_H

#include <iostream>
#include <libdevcore/Common.h>
#include <libdevcrypto/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/vector_ref.h>
#include <libdevcrypto/base58.h>
#include <libdevcore/SHA3.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libbrcdchain/Transaction.h>

using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace dev::brc::transationTool;
namespace js = json_spirit;

namespace wallet{

    struct trx_source {
        enum Contract
        {
            null =0,
            deploy =1,
            execute
        };
        Address from;
        Address to;
        u256 value;
        bytes data;
        u256 nonce = Invalid256;
        u256 gas = Invalid256;
        u256 gasPrice = Invalid256;
        u256 chainId = -4;
        std::vector<std::shared_ptr<operation>> ops;
        Contract isContract = Contract::null;
    };

    class ToolTransaction {
    public:
        ToolTransaction() = default;
        ~ToolTransaction() = default;

        static std::pair<bool, std::string> sign_trx_from_json(std::string json_str, std::string& transaction_hash);

        static std::string sendRawTransation(std::string const &_rlpStr, std::string const &_ip_port);

    private:
        static bytes packed_operation_data(const std::vector<std::shared_ptr<operation>> &op);

    };
}

#endif //BAERCHAIN_TOOLTRANSACTION_H
