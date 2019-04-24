#define BOOST_TEST_MODULE test_brc_db
#define BOOST_TEST_DYN_LINK
//#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/random.hpp>
#include <boost/format.hpp>
#include <libdevcrypto/base58.h>
#include <libbrcdchain/Transaction.h>

using namespace dev;
using namespace dev::brc::transationTool;

BOOST_AUTO_TEST_SUITE(test_pack_tx)

    BOOST_AUTO_TEST_CASE(pack_test_1) {

        std::map<Address, Secret>   address_secret;
        std::vector<bytes>  vec_trx;



        auto cancel_op = cancelPendingorder_operation(4, 3, h256("0xf42bfa5c6d13de747541078dcad69d331066c176685f579e730c961e1dd25ff0"));
        bytes data = cancel_op.serialize();


        for(auto i = 0 ; i < 1000; i++){
            auto key_pair = KeyPair::create();
            auto sec = key_pair.secret();
            address_secret[key_pair.address()] = key_pair.secret();


            brc::TransactionSkeleton ts;
            ts.creation = false;
            ts.from = key_pair.address();
            ts.to = key_pair.address();
            ts.value = u256(0xfffff);
            ts.data =   data;
            ts.nonce = u256(0);
            ts.gas = u256(2);
            ts.gasPrice = u256(1);

            brc::Transaction sign_t(ts, key_pair.secret());

            vec_trx.push_back(sign_t.rlp());
        }

        {
            Timer  t;
            for(auto &itr : vec_trx){
                dev::brc::Transaction t(itr, dev::brc::CheckTransaction::Everything);
            }

            std::cout << t.elapsed() << std::endl;
        }








    }


BOOST_AUTO_TEST_SUITE_END()