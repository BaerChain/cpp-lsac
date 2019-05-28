#pragma once
#include <brc/types.hpp>
#include <libdevcore/Address.h>
#include <libdevcore/Common.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>


#include <chainbase/chainbase.hpp>

//using namespace chainbase;
using namespace boost::multi_index;

namespace dev {
    namespace brc {
        namespace ex {

            typedef int64_t Time_ms;




            struct order {
                h256 trxid;
                Address sender;
                order_buy_type buy_type;
                order_token_type token_type;
                order_type type;
                std::map<u256, u256> price_token;
                Time_ms time;
            };


            struct result_order {
                result_order() {}

                template<typename ITR1, typename ITR2>
                result_order(const ITR1 &itr1, const ITR2 &itr2, const u256 &_amount, const u256 &_price) {
                    set_data(itr1, itr2, _amount, _price);
                }

                template<typename ITR1, typename ITR2>
                void set_data(const ITR1 &itr1, const ITR2 &itr2, const u256 &_amount, const u256 &_price) {
                    sender = itr1.sender;
                    acceptor = itr2->sender;
                    type = itr1.type;
                    token_type = itr1.token_type;
                    buy_type = itr1.buy_type;
                    create_time = itr1.time;
                    send_trxid = itr1.trxid;
                    to_trxid = itr2->trxid;
                    amount = _amount;
                    price = _price;
                }

                Address             sender;
                Address             acceptor;
                order_type          type;
                order_token_type    token_type;         //sender token type
                order_buy_type      buy_type;
                Time_ms             create_time;        //success time.
                h256                send_trxid;         //sender trxid;
                h256                to_trxid;           //which trxid
                u256                amount;
                u256                price;
            };



            ////////////////////////////////////////////////////////////////////////
            //////////////////////////////  object ///////////////////////////////
            ////////////////////////////////////////////////////////////////////////

            enum object_id {
                order_object_id = 0,
                order_result_object_id,
                dynamic_object_id
            };


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            class order_object : public chainbase::object<order_object_id, order_object> {
            public:
                template<typename Constructor, typename Allocator>
                order_object(Constructor &&c, Allocator &&a) {
                    c(*this);
                }

                id_type id;
                h256 trxid;
                Address sender;
                u256 price;
                u256 token_amount;
                u256 source_amount;
                Time_ms create_time;
                order_type type;
                order_token_type token_type;


                //set data
                template<typename ITR1, typename ITR2>
                void set_data(ITR1 itr, ITR2 t, u256 rel_amount) {
                    sender = itr.sender;
                    trxid = itr.trxid;
                    price = t.first;
                    token_amount = rel_amount;
                    source_amount = t.second;
                    create_time = itr.time;
                    type = itr.type;
                    token_type = itr.token_type;
                }

            };


            struct by_id;
            struct by_trx_id;
            struct by_price_less;            // price less
            struct by_price_greater;
            struct by_price_sell;           // price grater
            struct by_address;


            typedef multi_index_container<
                    order_object,
                    indexed_by<
                            ordered_unique<tag<by_id>,
                                    member<order_object, order_object::id_type, &order_object::id>
                            >,
                            ordered_unique<tag<by_trx_id>,
                                    composite_key<order_object,
                                            member<order_object, h256, &order_object::trxid>
                                    >,
                                    composite_key_compare<std::greater<h256>>
                            >,
                            ordered_non_unique<tag<by_price_less>,
                                    composite_key<order_object,
                                            member<order_object, order_type, &order_object::type>,
                                            member<order_object, order_token_type, &order_object::token_type>,
                                            member<order_object, u256, &order_object::price>,
                                            member<order_object, Time_ms, &order_object::create_time>
                                    >,
                                    composite_key_compare<std::less<order_type>, std::less<order_token_type>, std::less<u256>, std::less<Time_ms>>
                            >,
                            ordered_non_unique<tag<by_price_greater>,
                                    composite_key<order_object,
                                            member<order_object, order_type, &order_object::type>,
                                            member<order_object, order_token_type, &order_object::token_type>,
                                            member<order_object, u256, &order_object::price>,
                                            member<order_object, Time_ms, &order_object::create_time>
                                    >,
                                    composite_key_compare<std::less<order_type>, std::less<order_token_type>, std::greater<u256>, std::less<Time_ms>>
                            >,
                            ordered_non_unique<tag<by_address>,
                                    composite_key<order_object,
                                            member<order_object, Address, &order_object::sender>,
                                            member<order_object, Time_ms, &order_object::create_time>
                                    >,
                                    composite_key_compare<std::less<Address>, std::greater<Time_ms>>
                            >
                    >,
                    chainbase::allocator<order_object>
            > order_object_index;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            class order_result_object : public chainbase::object<order_result_object_id, order_result_object> {
            public:
                template<typename Constructor, typename Allocator>
                order_result_object(Constructor &&c, Allocator &&a) {
                    c(*this);
                }

                void set_data(const result_order &ret) {
                    sender = ret.sender;
                    acceptor = ret.acceptor;
                    type = ret.type;
                    token_type = ret.token_type;
                    buy_type = ret.buy_type;
                    create_time = ret.create_time;
                    send_trxid = ret.send_trxid;
                    to_trxid = ret.to_trxid;
                    amount = ret.amount;
                    price = ret.price;
                }

                id_type id;
                Address sender;
                Address acceptor;
                order_type type;
                order_token_type token_type;         //sender token type
                order_buy_type   buy_type;
                Time_ms create_time;                //success time.
                h256 send_trxid;                    //sender trxid;
                h256 to_trxid;                      //which trxid
                u256 amount;
                u256 price;

            };

            struct by_sender;
            struct by_greater_id;
            typedef multi_index_container<
                    order_result_object,
                    indexed_by<
                            ordered_unique<tag<by_greater_id>,
                                    member<order_result_object, order_result_object::id_type, &order_result_object::id>
                            >,
                            ordered_non_unique<tag<by_sender>,
                                    composite_key<order_result_object,
                                            member<order_result_object, Address, &order_result_object::sender>,
                                            member<order_result_object, Time_ms, &order_result_object::create_time>
                                    >,
                                    composite_key_compare<std::less<Address>, std::less<Time_ms>>
                            >
                    >,
                    chainbase::allocator<order_result_object>
            > order_result_object_index;




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            class dynamic_object : public chainbase::object<dynamic_object_id, dynamic_object> {
            public:
                template<typename Constructor, typename Allocator>
                dynamic_object(Constructor &&c, Allocator &&a) {
                    c(*this);
                }
                id_type id;
                int64_t version;
                uint64_t  orders;           //all orders numbers.
                uint64_t  result_orders;       // all exchange order .
            };


            typedef multi_index_container<
                    dynamic_object,
                    indexed_by<
                            ordered_unique<tag<by_id>,
                                    member<dynamic_object, dynamic_object::id_type, &dynamic_object::id>
                            >
                    >,
                    chainbase::allocator<dynamic_object>
            > dynamic_object_index;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


            struct exchange_order {
                exchange_order(const order_object &obj)
                        : trxid(obj.trxid), sender(obj.sender), price(obj.price), token_amount(obj.token_amount),
                          source_amount(obj.source_amount), create_time(obj.create_time), type(obj.type),
                          token_type(obj.token_type) {
                }

                h256 trxid;
                Address sender;
                u256 price;
                u256 token_amount;
                u256 source_amount;
                Time_ms create_time;
                order_type type;
                order_token_type token_type;
            };

        }
    }
}


CHAINBASE_SET_INDEX_TYPE(dev::brc::ex::order_object, dev::brc::ex::order_object_index)
CHAINBASE_SET_INDEX_TYPE(dev::brc::ex::order_result_object, dev::brc::ex::order_result_object_index)
CHAINBASE_SET_INDEX_TYPE(dev::brc::ex::dynamic_object, dev::brc::ex::dynamic_object_index)
