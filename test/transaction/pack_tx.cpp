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
#include <iostream>
#include <future>
#include <thread>
#include <vector>

using namespace dev;
using namespace dev::brc::transationTool;



template <typename T, typename R>
struct Task{
    typedef  std::vector<T>     value_source;
    typedef  std::vector<R>     value_result;

    Task(uint32_t numbers = 4):m_thread(numbers){

    }

    template <typename CONVERT>
    void go_task(const value_source &source, value_result &result, CONVERT conver_func){
        std::vector<std::vector<T>>  split_source(m_thread);
        std::vector<std::vector<R>>  split_result(m_thread);

        auto one_size = source.size() / m_thread;
        auto sp_size = source.size() % one_size;        //find surplus
        std::vector<std::future<value_result>> vec_ff;


        for(int i = 0; i < m_thread; i++){
            if(i == m_thread - 1 && sp_size != 0){  //add surplus to last thread
                split_source[i].assign(source.begin() + i * one_size, source.begin() + (i + 1) * one_size + sp_size);
            }else{
                split_source[i].assign(source.begin() + i * one_size, source.begin() + (i + 1) * one_size);
            }
            std::packaged_task<value_result(value_source)>  task([&](const value_source &d){
                value_result ret;
                for(auto itr : d){
                    ret.push_back( conver_func(itr));

                }
                return ret;
            });
            vec_ff.push_back(task.get_future());
            std::thread(std::move(task), split_source[i]).detach();
        }


        for(int i = 0; i < m_thread; i++){
            vec_ff[i].wait();
            split_result[i] = std::move(vec_ff[i].get());
        }

        for(auto i = 0 ; i < m_thread; i++){
            result.insert(result.end(), split_result[i].begin(), split_result[i].end());
        }


    }

private:
    uint32_t  m_thread;
};






BOOST_AUTO_TEST_SUITE(test_pack_tx)

    BOOST_AUTO_TEST_CASE(pack_test_1) {

        std::map<Address, Secret>   address_secret;
        std::vector<bytes>  vec_trx;



        auto cancel_op = cancelPendingorder_operation(4, 3, h256("0xf42bfa5c6d13de747541078dcad69d331066c176685f579e730c961e1dd25ff0"));
        bytes data = cancel_op.serialize();


        for(auto i = 0 ; i < 10001; i++){
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
            std::vector<dev::brc::Transaction> ret1;
            for(auto &itr : vec_trx){
                dev::brc::Transaction t(itr, dev::brc::CheckTransaction::Everything);
                ret1.push_back(t);
            }

            std::cout << t.elapsed() << std::endl;
            for(auto count = 1; count < 12; count++){
                t.restart();
                Task<bytes, dev::brc::Transaction> task(count);
                std::vector<dev::brc::Transaction> ret;
                task.go_task(vec_trx, ret, [](const bytes &b){
                    return dev::brc::Transaction(b, dev::brc::CheckTransaction::Everything);
                });

                std::cout << "t : " << count << "  " << t.elapsed() << std::endl;

                BOOST_CHECK(ret1 == ret);
            }

        }







    }


BOOST_AUTO_TEST_SUITE_END()