//
// Created by friday on 2019/8/19.
//

#include "ExdbState.h"


namespace dev {
    namespace brc {


        std::vector<ex::result_order> ExdbState::insert_operation(const ex::ex_order &itr, bool reset) {


            if(itr.token_amount == 0 && itr.price == 0){
                BOOST_THROW_EXCEPTION(order_price_num_equal_zero_error());
            }

            std::vector<result_order> result;
            if(reset){
                if(!(itr.type == order_type::buy && itr.buy_type == order_buy_type::all_price)){
                    cwarn << "reset only use all_price  && buy";
                    exit(1);
                }
            }
            bool throw_exception = true;
            if (itr.buy_type == order_buy_type::only_price) {
                if (itr.type == order_type::buy) {
                    auto find_itr = get_buy_itr(itr.token_type, itr.price);
                    process_only_price(find_itr.first, find_itr.second, itr, itr.price,
                                       itr.source_amount,
                                       result,
                                       throw_exception);

                } else { //sell
                    auto find_itr = get_sell_itr(itr.token_type, itr.price);
                    process_only_price(find_itr.first, find_itr.second, itr, itr.price,
                                       itr.source_amount,
                                       result,
                                       throw_exception);
                }

            } else {
                if (itr.type == order_type::buy) {
                    assert(itr.price != 0 && itr.source_amount == 0);

                    auto find_itr = get_buy_itr(itr.token_type, u256(-1));
                    auto total_price = itr.price;
                    auto begin = find_itr.first;
                    auto end = find_itr.second;
                    if (begin != end) {
                        while (total_price > 0 && begin != end) {
                            auto begin_total_price = begin->token_amount * begin->price;
                            result_order ret;
                            if (begin_total_price <= total_price) {   //
                                total_price -= begin_total_price;
                                ret.set_data(itr, begin, begin->token_amount, begin->price);
                                result.push_back(ret);

                                if(!reset){
                                    add_resultOrder(ret);
                                    remove_exchangeOrder(begin->trxid);
                                }



                            } else if (begin_total_price > total_price) {
                                auto can_buy_amount = total_price / begin->price;
                                if (can_buy_amount == 0) {
                                    break;
                                }
                                ret.set_data(itr, begin, can_buy_amount, begin->price);
                                result.push_back(ret);

                                if(!reset){
                                    add_resultOrder(ret);
                                    auto data_update = *begin;
                                    data_update.token_amount -= can_buy_amount;
                                    add_exchangeOrder(data_update);
                                }


                                break;
                            }
                        }
                    } else {
                        BOOST_THROW_EXCEPTION(all_price_operation_error());
                    }
                } else {   //all_price  , sell,
                    assert(itr.price == 0 && itr.source_amount != 0);

                    auto find_itr = get_sell_itr(itr.token_type, u256(0));
                    auto begin = find_itr.first;
                    auto end = find_itr.second;
                    auto total_amount = itr.price;
                    if (begin != end) {
                        while (total_amount > 0 && begin != end) {
                            result_order ret;
                            if (begin->token_amount > total_amount) {
                                ret.set_data(itr, begin, total_amount, begin->price);
                                result.push_back(ret);

                                auto data_update = *begin;
                                data_update.token_amount -= total_amount;
                                add_exchangeOrder(data_update);

                                total_amount = 0;
                            } else {
                                total_amount -= begin->token_amount;
                                ret.set_data(itr, begin, begin->token_amount, begin->price);
                                result.push_back(ret);

                                remove_exchangeOrder(begin->trxid);


                            }
                            add_resultOrder(ret);
                        }
                    } else {
                        BOOST_THROW_EXCEPTION(all_price_operation_error());
                    }
                }
            }
            return result;
        }


        template<typename BEGIN, typename END>
        void ExdbState::process_only_price(BEGIN &begin, END &end, const ex_order &od, const u256 &price,
                                           const u256 &amount,
                                           std::vector<result_order> &result, bool throw_exception) {

            if (begin == end) {
                add_exchangeOrder(od);
                return;
            }
            auto spend = amount;

            bool rm = false;
            while (spend > 0 && begin != end) {
                result_order ret;
                if (begin->token_amount <= spend) {
                    spend -= begin->token_amount;
                    ret.set_data(od, begin, begin->token_amount, begin->price);
                    rm = true;

                } else {
                    auto update = *begin;
                    update.token_amount -= spend;

                    //update data.
                    add_exchangeOrder(od);

                    ret.set_data(od, begin, spend, begin->price);
                    spend = 0;
                }
                ret.old_price = price;

                add_resultOrder(ret);

                result.push_back(ret);
                if (rm) {
                    remove_exchangeOrder(begin->trxid);
                } else {
                    begin++;
                }

            }
            //surplus token ,  record to db
            if (spend > 0) {
                ex_order exod = od;
                exod.token_amount = spend;
                add_exchangeOrder(exod);
            }
        }


        ex::order ExdbState::cancel_order_by_trxid(const dev::h256 &t) {
            const auto &index_trx = m_state.getExOrder().get<ex_by_trx_id>();
            cdebug << "remove tx id " << toHex(t);
            auto begin = index_trx.lower_bound(t);
            auto end = index_trx.upper_bound(t);
            if (begin == end) {
                BOOST_THROW_EXCEPTION(find_order_trxid_error() << errinfo_comment(toString(t)));
            }

            order o;
            o.trxid = begin->trxid;
            o.sender = begin->sender;
            o.buy_type = order_buy_type::only_price;
            o.token_type = begin->token_type;
            o.type = begin->type;
            o.time = begin->create_time;
            o.price_token.first = begin->price;
            o.price_token.second = begin->token_amount;


            remove_exchangeOrder(begin->trxid);

            return o;
        }

        void ExdbState::add_exchangeOrder(const ex_order &od) {
            m_state.addExchangeOrder(ExdbSystemAddress, od);
        }


        void ExdbState::remove_exchangeOrder(const dev::h256 &id) {
            m_state.removeExchangeOrder(ExdbSystemAddress, id);
        }

        void ExdbState::add_resultOrder(const dev::brc::result_order &od) {
            m_state.addSuccessExchange(od);
        }

        std::vector<exchange_order> ExdbState::get_order_by_address(const Address &addr) const {

            std::vector<exchange_order> ret;

            const auto &index = m_state.getExOrder().get<ex_by_address>();
            auto lower_itr = index.lower_bound(boost::tuple<Address, Time_ms>(addr, INT64_MAX));
            auto up_itr = index.upper_bound(boost::tuple<Address, Time_ms>(addr, 0));
            while (lower_itr != up_itr && lower_itr != index.end()) {
                ret.push_back(ex::exchange_order(*lower_itr));
                lower_itr++;
            }

            return ret;

        }
//
//        std::vector<exchange_order> ExdbState::get_orders(uint32_t size) const {
//            std::vector<exchange_order> ret;
//            const auto &index = m_state.getExOrder().get<ex_by_price_less>();
//            auto begin = index.begin();
//            while (begin != index.end() && size > 0) {
//                ret.push_back(exchange_order(*begin));
//                begin++;
//                size--;
//            }
//            return ret;
//        }

        std::vector<result_order> ExdbState::get_result_orders_by_news(uint32_t size) const {
            std::vector<result_order> ret;
            const auto &index = m_state.getSuccessExchange().get<ex_by_time>();
            auto begin = index.begin();
            while (begin != index.end() && size > 0) {
                ex::result_order eo;

                eo.sender = begin->sender;
                eo.acceptor = begin->acceptor;
                eo.type = begin->type;
                eo.token_type = begin->token_type;
                eo.buy_type = begin->buy_type;
                eo.create_time = begin->create_time;
                eo.send_trxid = begin->send_trxid;
                eo.to_trxid = begin->to_trxid;
                eo.amount = begin->amount;
                eo.price = begin->price;

                ret.push_back(eo);
                begin++;
                size--;
            }
            return ret;
        }

        std::vector<result_order>
        ExdbState::get_result_orders_by_address(const Address &addr, int64_t min_time, int64_t max_time,
                                                uint32_t max_size) const {
            std::vector<result_order> ret;
            std::vector<result_order> ret_sender;
            std::vector<result_order> ret_acceptor;



//            uint32_t size = max_size;
//            {
//                const auto &sender_index = m_state.getSuccessExchange().get<ex_by_sender>();
//                auto sender_lower_itr = sender_index.lower_bound(
//                        boost::tuple<Address, Time_ms>(addr, min_time));
//                auto sender_up_itr = sender_index.upper_bound(boost::tuple<Address, Time_ms>(addr, max_time));
//                while (sender_lower_itr != sender_up_itr && size-- > 0) {
//                    result_order eo;
//                    eo.sender = sender_lower_itr->sender;
//                    eo.acceptor = sender_lower_itr->acceptor;
//                    eo.type = sender_lower_itr->type;
//                    eo.token_type = sender_lower_itr->token_type;
//                    eo.buy_type = sender_lower_itr->buy_type;
//                    eo.create_time = sender_lower_itr->create_time;
//                    eo.send_trxid = sender_lower_itr->send_trxid;
//                    eo.to_trxid = sender_lower_itr->to_trxid;
//                    eo.amount = sender_lower_itr->amount;
//                    eo.price = sender_lower_itr->price;
//
//                    ret_sender.push_back(eo);
//                    sender_lower_itr++;
//                }
//            }
//
//            {
//                const auto &acceptor_index = m_state.getSuccessExchange().get<ex_by_acceptor>();
//                auto acceptor_lower_itr = acceptor_index.lower_bound(
//                        boost::tuple<Address, Time_ms>(addr, min_time));
//                auto acceptor_up_itr = acceptor_index.upper_bound(
//                        boost::tuple<Address, Time_ms>(addr, max_time));
//
//                size = max_size;
//                while (acceptor_lower_itr != acceptor_up_itr && size-- > 0) {
//                    result_order eo;
//                    eo.sender = acceptor_lower_itr->sender;
//                    eo.acceptor = acceptor_lower_itr->acceptor;
//                    eo.type = acceptor_lower_itr->type;
//                    eo.token_type = acceptor_lower_itr->token_type;
//                    eo.buy_type = acceptor_lower_itr->buy_type;
//                    eo.create_time = acceptor_lower_itr->create_time;
//                    eo.send_trxid = acceptor_lower_itr->send_trxid;
//                    eo.to_trxid = acceptor_lower_itr->to_trxid;
//                    eo.amount = acceptor_lower_itr->amount;
//                    eo.price = acceptor_lower_itr->price;
//
//                    ret_acceptor.push_back(eo);
//                    acceptor_lower_itr++;
//                }
//
//            }
//
//
//            //sort
//            {
//                size = 0;
//                auto sender_itr = ret_sender.begin();
//                auto acceptor_itr = ret_acceptor.begin();
//                for (; size < max_size &&
//                       (sender_itr != ret_sender.end() || acceptor_itr != ret_acceptor.end());
//                       size++) {
//                    if (sender_itr != ret_sender.end() && acceptor_itr != ret_acceptor.end()) {
//                        if (sender_itr->create_time > acceptor_itr->create_time) {
//                            ret.push_back(*sender_itr);
//                            sender_itr++;
//                        } else {
//                            ret.push_back(*acceptor_itr);
//                            acceptor_itr++;
//                        }
//                    } else if (sender_itr != ret_sender.end()) {
//                        ret.push_back(*sender_itr);
//                        sender_itr++;
//                    } else {
//                        ret.push_back(*acceptor_itr);
//                        acceptor_itr++;
//                    }
//                }
//            }
            return ret;
        }

        std::vector<exchange_order>
        ExdbState::get_order_by_type(order_type type, order_token_type token_type, uint32_t size) const {
            std::vector<exchange_order> ret;
            if (type == order_type::buy) {
                const auto &index_greater = m_state.getExOrder().get<ex_by_price_greater>();
                auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::buy,
                                                                                            token_type,
                                                                                            u256(-1), 0);
                auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::buy,
                                                                                            token_type,
                                                                                            u256(0), INT64_MAX);
                auto begin = index_greater.lower_bound(find_lower);
                auto end = index_greater.upper_bound(find_upper);

                while (begin != end && size > 0) {
                    ret.push_back(exchange_order(*begin));
                    begin++;
                    size--;
                }
            } else {
                const auto &index_less = m_state.getExOrder().get<ex_by_price_less>();
                auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::sell,
                                                                                            token_type,
                                                                                            u256(0), 0);
                auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::sell,
                                                                                            token_type,
                                                                                            u256(-1), INT64_MAX);
                auto begin = index_less.lower_bound(find_lower);
                auto end = index_less.upper_bound(find_upper);
                while (begin != end && size > 0) {
                    ret.push_back(exchange_order(*begin));
                    begin++;
                    size--;
                }
            }
            return ret;
        }
//
        std::vector<ex_order> ExdbState::exits_trxid(const h256 &t) {

            const auto &index_trx = m_state.getExOrder().get<ex_by_trx_id>();
            cdebug << "remove tx id " << toHex(t);
            auto begin = index_trx.lower_bound(t);
            auto end = index_trx.upper_bound(t);
            if (begin == end) {
                BOOST_THROW_EXCEPTION(find_order_trxid_error() << errinfo_comment(toString(t)));
            }

            ex_order o = *begin;
//            o.trxid = begin->trxid;
//            o.sender = begin->sender;
//            o.buy_type = order_buy_type::only_price;
//            o.token_type = begin->token_type;
//            o.type = begin->type;
//            o.create_time = begin->create_time;
//            o.price = begin->price;
//            o.token_amount = begin->token_amount;
//            o.source_amount = begin->source_amount;

            return {o};
        }


    }
}