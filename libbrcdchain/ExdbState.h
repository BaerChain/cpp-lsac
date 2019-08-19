//
// Created by friday on 2019/8/19.
//

#ifndef BAERCHAIN_EXDBSTATE_H
#define BAERCHAIN_EXDBSTATE_H

#include "State.h"
#include <libdevcore/Common.h>
#include <libdevcore/Address.h>
#include <libdevcrypto/Common.h>
#include <libbvm/ExtVMFace.h>
#include <libbrccore/config.h>
#include <brc/objects.hpp>





namespace dev{
    namespace brc{

        using namespace ex;

        class ExdbState {
        public:
            ExdbState(State &_state):m_state(_state){}

            /// insert operation .
            /// \param orders   vector<order>
            /// \param reset    if true , this operation will reset, dont record.
            /// \param throw_exception  if true  throw exception when error.
            /// \return
            std::vector<ex::result_order> insert_operation(const ex::ex_order &od, bool reset = false);

            ///
            /// \param os vector transactions id
            /// \param reset   if true, this operation rollback
            /// \return        vector<orders>
            ex::order  cancel_order_by_trxid(const h256 &id);


            /// get exchange order by address,
            /// \param addr   Address
            /// \return         complete order.
//            std::vector<exchange_order> get_order_by_address(const Address &addr) const;
//
//            /// get current all orders on exchange by size. this search by id in function.
//            /// \param size  once get once.
//            /// \return
//            std::vector<exchange_order> get_orders(uint32_t size = 50) const;
//
//            /// get newest result_order by size
//            /// \param size         once get size.
//            /// \return             vector<result_orders>.
////            std::vector<result_order> get_result_orders_by_news(uint32_t size = 50) const;
//
//
//            /// get result_order of address by time, size must < 50
//            /// \param addr         which Address
//            /// \param min_time     min time
//            /// \param max_time     max time
//            /// \param max_size     once get size.
//            /// \return             result_order
//            std::vector<result_order> get_result_orders_by_address(const Address &addr, int64_t min_time, int64_t max_time, uint32_t max_size = 50) const;
//
//
            /// get exchange order by type (sell or buy && BRC or  FUEL)
            /// \param type         sell or buy
            /// \param token_type   BRC OR FUEL
            /// \param size         once search size.
            /// \return             complete order.
            std::vector<ex::exchange_order> get_order_by_type(order_type type, order_token_type token_type, uint32_t size) const;
//

            std::vector<ex_order> exits_trxid(const h256  &trxid);

        private:

            //add or update
            void add_exchangeOrder(const ex_order &od);

            void remove_exchangeOrder(const h256 &id);

            void add_resultOrder(const result_order &od);



            /// get iterator by type and price . this only find order of sell.
            /// \param token_type   BRC OR FUEL
            /// \param price        upper price.
            /// \return         std::pair<lower iterator, upper iterator>
            auto get_buy_itr(ex::order_token_type find_token, u256 price) {
//                    auto find_token = token_type == order_token_type::BRC ? order_token_type::FUEL : order_token_type::BRC;
                const auto &index_greater = m_state.getExOrder().get<ex::ex_by_price_less>();

                auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::sell, find_token,
                                                                                            u256(0), 0);
                auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::sell, find_token,
                                                                                            price, INT64_MAX);

                typedef decltype(index_greater.lower_bound(find_lower)) Lower_Type;
                typedef decltype(index_greater.upper_bound(find_upper)) Upper_Type;

                return std::pair<Lower_Type, Upper_Type>(index_greater.lower_bound(find_lower),
                                                         index_greater.upper_bound(find_upper));
            };



            /// get iterator by type and price, this only find of buy
            /// \param token_type   BRC OR FUEL,
            /// \param price        lower price.
            /// \return             std::pair<lower iterator, upper iterator>
            auto get_sell_itr(order_token_type find_token, u256 price) {
//                    auto find_token = token_type == order_token_type::BRC ? order_token_type::FUEL : order_token_type::BRC;
                const auto &index_less = m_state.getExOrder().get<ex_by_price_greater>();  //↑

                auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::buy, find_token,
                                                                                            u256(-1), 0);
                auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::buy, find_token,
                                                                                            price, INT64_MAX);

                typedef decltype(index_less.lower_bound(find_lower)) Lower_Type;
                typedef decltype(index_less.upper_bound(find_upper)) Upper_Type;

                return std::pair<Lower_Type, Upper_Type>(index_less.lower_bound(find_lower),
                                                         index_less.upper_bound(find_upper));
            };



            /// process buy or sell orders by price and amount,
            /// \tparam BEGIN   get_buy_itr.first or get_sell_itr.first.
            /// \tparam END     get_buy_itr.second or get_buy_itr.second.
            /// \param begin    begin iterator,
            /// \param end      end iterator.
            /// \param od       source order.
            /// \param price    price,
            /// \param amount   exchange amount
            /// \param result   result of success order.
            /// \param throw_exception  if true, will throw exception while error.
            template<typename BEGIN, typename END>
            void process_only_price(BEGIN &begin, END &end, const ex::ex_order &od, const u256 &price, const u256 &amount,
                                    std::vector<ex::result_order> &result, bool throw_exception);


            State  &m_state;
        };



    }
}

#endif //BAERCHAIN_EXDBSTATE_H
