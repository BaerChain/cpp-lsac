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
#include <boost/thread.hpp>
#include <boost/lockfree/queue.hpp>

using namespace dev;
using namespace dev::brc::transationTool;


template<typename T, typename R>
struct Task {
    typedef std::vector<T> value_source;
    typedef std::vector<R> value_result;
    typedef typename std::vector<T>::const_iterator Iterator;

    Task(uint32_t numbers = 4) : m_thread(numbers) {

    }

    template<typename CONVERT>
    void go_task(const value_source &source, value_result &result, CONVERT conver_func) {
        std::vector<std::vector<T>> split_source(m_thread);
        std::vector<std::vector<R>> split_result(m_thread);

        auto one_size = source.size() / m_thread;
        auto sp_size = source.size() % one_size;        //find surplus
        std::vector<std::future<value_result>> vec_ff;


        for (int i = 0; i < m_thread; i++) {


            std::packaged_task<value_result(Iterator, Iterator)> task([&](Iterator begin, Iterator end) -> value_result{
                value_result ret;
                while (begin != end) {
                    ret.push_back(conver_func(*begin));
                    begin++;
                }
                return ret;
            });
            vec_ff.push_back(task.get_future());
            if (i == m_thread - 1 && sp_size != 0) {  //add surplus to last thread
//                split_source[i].assign(source.begin() + i * one_size, source.begin() + (i + 1) * one_size + sp_size);
                std::thread(std::move(task), source.begin() + i * one_size, source.end()).detach();
            } else {
//                split_source[i].assign(source.begin() + i * one_size, source.begin() + (i + 1) * one_size);
                std::thread(std::move(task), source.begin() + i * one_size, source.begin() + + (i + 1) * one_size).detach();
            }
        }


        for (int i = 0; i < m_thread; i++) {
            vec_ff[i].wait();
            split_result[i] = std::move(vec_ff[i].get());
        }

        for (auto i = 0; i < m_thread; i++) {
            result.insert(result.end(), split_result[i].begin(), split_result[i].end());
        }


    }

private:
    uint32_t m_thread;
};


template<typename T, typename R>
struct task_work {

    typedef std::vector<T> source_value;
    typedef std::vector<R> result_value;

    typedef typename std::vector<T>::const_iterator Iterator;


    typedef std::function<R(const T &)> CONVERT;

    struct request_context {
        Iterator begin;
        Iterator end;
        result_value *output = nullptr;
        CONVERT ff = nullptr;
        boost::promise<void> *ret = nullptr;

        bool empty() {
            return
//            input != nullptr &&
                    output != nullptr &&
                    ret != nullptr;
        }

    };

    task_work(uint32_t threads) : m_thread(threads), m_queue(threads * 2), m_running(true), m_sleep_times(1000 * 1) {
        for (auto i = 0; i < threads; i++) {
            m_th.push_back(boost::thread([&]() {
                request_context *cxt = nullptr;
                while (m_running) {

                    while (m_queue.pop(cxt)) {
                        if (cxt->empty()) {
                            while (cxt->begin != cxt->end) {
                                cxt->output->emplace_back(std::move(cxt->ff(*cxt->begin)));
                                cxt->begin++;
                            }


                            cxt->ret->set_value();
                        } else {
                            if (cxt->ret) {
                                cxt->ret->set_value();
                            }
                        }
                    }
                    boost::this_thread::sleep(boost::posix_time::microseconds(m_sleep_times));
                }
            }));
            m_th[i].detach();
        }
    }


    void add_task(const source_value &source, result_value &result, CONVERT f) {
        std::vector<std::vector<T>> split_source(m_thread);
        std::vector<std::vector<R>> split_result(m_thread);

        auto one_size = source.size() / m_thread;
        auto sp_size = source.size() % one_size;        //find surplus

        std::vector<boost::promise<void> > proms(m_thread);
        std::vector<request_context> contexts(m_thread);
        for (int i = 0; i < m_thread; i++) {
            if (i == m_thread - 1 && sp_size != 0) {  //add surplus to last thread
                contexts[i].begin = source.begin() + i * one_size;
                contexts[i].end = source.end();
            } else {
                contexts[i].begin = source.begin() + i * one_size;
                contexts[i].end = source.begin() + (i + 1) * one_size;
            }
            contexts[i].ff = f;
            contexts[i].output = &split_result[i];
            contexts[i].ret = &proms[i];
            m_queue.push(&contexts[i]);
        }

        for (auto &itr : proms) {
            itr.get_future().get();
        }
        for (auto i = 0; i < m_thread; i++) {
            result.insert(result.end(), split_result[i].begin(), split_result[i].end());
        }
    }

private:
    uint32_t m_thread;
    boost::lockfree::queue<request_context *> m_queue;
    std::vector<boost::thread> m_th;
    uint32_t m_sleep_times;      //micrseconds
    bool m_running;
};


BOOST_AUTO_TEST_SUITE(test_pack_tx)

    BOOST_AUTO_TEST_CASE(pack_test_1) {

        std::map<Address, Secret> address_secret;
        std::vector<bytes> vec_trx;


        auto cancel_op = cancelPendingorder_operation(4, 3,
                                                      h256("0xf42bfa5c6d13de747541078dcad69d331066c176685f579e730c961e1dd25ff0"));
        bytes data = cancel_op.serialize();


        for (auto i = 0; i < 1001; i++) {
            auto key_pair = KeyPair::create();
            auto sec = key_pair.secret();
            address_secret[key_pair.address()] = key_pair.secret();


            brc::TransactionSkeleton ts;
            ts.creation = false;
            ts.from = key_pair.address();
            ts.to = key_pair.address();
            ts.value = u256(0xfffff);
            ts.data = data;
            ts.nonce = u256(0);
            ts.gas = u256(2);
            ts.gasPrice = u256(1);

            brc::Transaction sign_t(ts, key_pair.secret());

            vec_trx.push_back(sign_t.rlp());
        }


        Timer t;
        std::vector<dev::brc::Transaction> ret1;
        for (auto &itr : vec_trx) {
            dev::brc::Transaction t(itr, dev::brc::CheckTransaction::Everything);
            ret1.push_back(t);
        }

        std::cout << t.elapsed() << std::endl;
        {
            Timer all;
            Timer t;

//            for (auto count = 1; count < 100; count++) {
            t.restart();
            Task<bytes, dev::brc::Transaction> task(6);
            std::vector<dev::brc::Transaction> ret;
            task.go_task(vec_trx, ret, [](const bytes &b) {
                return dev::brc::Transaction(b, dev::brc::CheckTransaction::Cheap);
            });

//            }

            std::cout << "all : " << all.elapsed() << std::endl;
            BOOST_CHECK(ret1 == ret);
        }

        {
            Timer all;
            Timer t;
            task_work<bytes, dev::brc::Transaction> tw(6);
//            for (auto count = 1; count < 100; count++) {
            t.restart();
            std::vector<dev::brc::Transaction> ret;
            tw.add_task(vec_trx, ret, [](const bytes &b) {
                return dev::brc::Transaction(b, dev::brc::CheckTransaction::Cheap);
            });

//            }
            std::cout << "all : " << all.elapsed() << std::endl;
            BOOST_CHECK(ret1 == ret);
        }


    }

    BOOST_AUTO_TEST_CASE(pack_test_2) {
        std::map<Address, Secret> address_secret;
        auto cancel_op = cancelPendingorder_operation(4, 3,
                                                      h256("0xf42bfa5c6d13de747541078dcad69d331066c176685f579e730c961e1dd25ff0"));
        bytes data = cancel_op.serialize();
        auto key_pair = KeyPair::create();
        auto sec = key_pair.secret();
        address_secret[key_pair.address()] = key_pair.secret();


        brc::TransactionSkeleton ts;
        ts.creation = false;
        ts.from = key_pair.address();
        ts.to = key_pair.address();
        ts.value = u256(0xfffff);
        ts.data = data;
        ts.nonce = u256(0);
        ts.gas = u256(2);
        ts.gasPrice = u256(1);

        brc::Transaction sign_t(ts, key_pair.secret());

        auto tx_rlp = sign_t.rlp();
        Timer t;
        for (int i = 0; i < 1; i++) {
            dev::brc::Transaction(tx_rlp, dev::brc::CheckTransaction::Everything);
        }

        std::cout << "all : " << t.elapsed() << std::endl;

    }

BOOST_AUTO_TEST_SUITE_END()