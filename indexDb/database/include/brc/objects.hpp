#pragma once
#include <brc/types.hpp>
#include <libdevcore/Address.h>
#include <libdevcore/Common.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>


#include <chainbase/chainbase.hpp>
#include <libdevcore/RLP.h>

//using namespace chainbase;
using namespace boost::multi_index;



namespace dev {
    namespace brc {

        namespace ex {

            typedef int64_t Time_ms;



            /*
             *  @param token_type : only cook
             *  @param price_token :    if all_price && buy brc :   price == 0, token > 0;
             *                          if all_price && sell brc :  price > 0, token == 0;
             *  */
            struct order {
                h256 trxid;
                Address sender;
                order_buy_type buy_type;
                order_token_type token_type;
                order_type type;
                std::pair<u256, u256> price_token;
                Time_ms time;
            };


            struct ex_order {
                h256 trxid;
                Address sender;
                u256 price;
                u256 token_amount;
                u256 source_amount;
                Time_ms create_time;
                order_type type;
                order_token_type token_type;
                order_buy_type buy_type;

                bytes streamRLP() const {
                    RLPStream s(9);
                    s << trxid << sender << price << token_amount << source_amount
                                <<(u256)create_time << (uint8_t)type<< (uint8_t)token_type<< (uint8_t)buy_type;
                    return s.out();
                }
                void populate(bytes const& b){
                    RLP rlp(b);
                    trxid = rlp[0].convert<h256>(RLP::LaissezFaire);
                    sender = rlp[1].convert<Address>(RLP::LaissezFaire);
                    price = rlp[2].convert<u256>(RLP::LaissezFaire);
                    token_amount = rlp[3].convert<u256>(RLP::LaissezFaire);
                    source_amount = rlp[4].convert<u256>(RLP::LaissezFaire);
                    create_time = (int64_t)rlp[5].convert<u256>(RLP::LaissezFaire);
                    type = (order_type)rlp[6].convert<uint8_t>(RLP::LaissezFaire);
                    token_type = (order_token_type)rlp[7].convert<uint8_t>(RLP::LaissezFaire);
                    buy_type = (order_buy_type)rlp[8].convert<uint8_t>(RLP::LaissezFaire);

                }
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
                    create_time = itr1.create_time;
                    send_trxid = itr1.trxid;
                    to_trxid = itr2->trxid;
                    amount = _amount;
                    price = _price;
                }

                bytes streamRLP() const{
                    RLPStream s(11);
                    s << sender << acceptor << (uint8_t)type << (uint8_t) token_type <<  (uint8_t) buy_type
                        << (u256) create_time << send_trxid << to_trxid << amount << price << old_price;
                    return  s.out();
                }
                void populate(bytes const& b){
                    RLP rlp(b);
                    sender = rlp[0].convert<Address>(RLP::LaissezFaire);
                    acceptor = rlp[1].convert<Address>(RLP::LaissezFaire);
                    type = (order_type)rlp[2].convert<uint8_t>(RLP::LaissezFaire);
                    token_type = (order_token_type)rlp[3].convert<uint8_t>(RLP::LaissezFaire);
                    buy_type = (order_buy_type)rlp[4].convert<uint8_t>(RLP::LaissezFaire);
                    create_time = (int64_t)rlp[5].convert<u256>(RLP::LaissezFaire);
                    send_trxid = rlp[6].convert<h256>(RLP::LaissezFaire);
                    to_trxid = rlp[7].convert<h256>(RLP::LaissezFaire);
                    amount = rlp[8].convert<u256>(RLP::LaissezFaire);
                    price = rlp[9].convert<u256>(RLP::LaissezFaire);
                    old_price = rlp[10].convert<u256>(RLP::LaissezFaire);
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
				u256                old_price;
            };


            struct ex_by_trx_id;
            struct ex_by_price_less;
            struct ex_by_price_greater;
            struct ex_by_address;

            typedef multi_index_container<
                    ex_order,
                    indexed_by<
                            ordered_non_unique<tag<ex_by_trx_id>,
                                    composite_key<ex_order, member<ex_order, h256, &ex_order::trxid>
                                    >,
                                    composite_key_compare<std::greater<h256>>
                            >,
                            ordered_non_unique<tag<ex_by_price_less>,
                                    composite_key<ex_order,
                                            member<ex_order, order_type, &ex_order::type>,
                                            member<ex_order, order_token_type, &ex_order::token_type>,
                                            member<ex_order, u256, &ex_order::price>,
                                            member<ex_order, Time_ms, &ex_order::create_time>
                                    >,
                                    composite_key_compare<std::less<order_type>, std::less<order_token_type>, std::less<u256>, std::less<Time_ms>>
                            >,
                            ordered_non_unique<tag<ex_by_price_greater>,
                                    composite_key<ex_order,
                                            member<ex_order, order_type, &ex_order::type>,
                                            member<ex_order, order_token_type, &ex_order::token_type>,
                                            member<ex_order, u256, &ex_order::price>,
                                            member<ex_order, Time_ms, &ex_order::create_time>
                                    >,
                                    composite_key_compare<std::less<order_type>, std::less<order_token_type>, std::greater<u256>, std::less<Time_ms>>
                            >,
                            ordered_non_unique<tag<ex_by_address>,
                                    composite_key<ex_order,
                                            member<ex_order, Address, &ex_order::sender>,
                                            member<ex_order, Time_ms, &ex_order::create_time>
                                    >,
                                    composite_key_compare<std::less<Address>, std::greater<Time_ms>>
                            >
                    >,
                    std::allocator<ex_order>
            > ExOrderMulti;



            struct ex_by_sender;
            struct ex_by_acceptor;
            struct ex_by_time;
            typedef multi_index_container<
                    result_order,
                    indexed_by<
                            ordered_non_unique<tag<ex_by_sender>,
                                    composite_key<result_order,
                                            member<result_order, Address, &result_order::sender>,
                                            member<result_order, Time_ms, &result_order::create_time>
                                    >,
                                    composite_key_compare<std::less<Address>, std::less<Time_ms>>
                            >,
                            ordered_non_unique<tag<ex_by_acceptor>,
                                    composite_key<result_order,
                                            member<result_order, Address, &result_order::acceptor>,
                                            member<result_order, Time_ms, &result_order::create_time>
                                    >,
                                    composite_key_compare<std::less<Address>, std::less<Time_ms>>
                            >,
                            ordered_unique<tag<ex_by_time>,
                                    composite_key<result_order,
                                            member<result_order, Time_ms, &result_order::create_time>
                                    >,
                                    composite_key_compare<std::less<Time_ms>>
                            >
                    >,
                    std::allocator<result_order>
            > ExResultOrder;



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
                            ordered_non_unique<tag<by_trx_id>,
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
            struct by_acceptor;
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
                            >,
                            ordered_non_unique<tag<by_acceptor>,
                                    composite_key<order_result_object,
                                            member<order_result_object, Address, &order_result_object::acceptor>,
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
                h256        block_hash;
                h256        root_hash;
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

                exchange_order(const ex_order &obj)
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
