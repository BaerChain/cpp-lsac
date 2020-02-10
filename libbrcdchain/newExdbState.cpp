#include "newExdbState.h"


namespace dev {
    namespace brc {


        std::vector<ex::result_order> newExdbState::insert_operation(const ex::ex_order &itr, bool reset) {


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

//            cwarn << "************************************************************" << itr.format_string();
//            auto vector_orders = get_order_by_type(order_type::sell, order_token_type::FUEL, 30);
//            auto vector_orders1 = get_order_by_type(order_type::buy, order_token_type::FUEL, 30);
//
//            for(auto & itr : vector_orders){
//                cwarn << itr.format_string();
//            }
//
//            for(auto & itr : vector_orders1){
//                cwarn << itr.format_string();
//            }
//            cwarn << "************************************************************";

            bool throw_exception = true;
            if (itr.buy_type == order_buy_type::only_price) {
                if (itr.type == order_type::buy) {
                    auto find_itr = get_buy_itr(itr.create_time, itr.price);
                    process_only_price(find_itr.first, find_itr.second, itr, itr.price,
                                       itr.source_amount,
                                       result,
                                       throw_exception);

                } else { //sell
                    auto find_itr = get_sell_itr(itr.create_time, itr.price);
                    process_only_price(find_itr.first, find_itr.second, itr, itr.price,
                                       itr.source_amount,
                                       result,
                                       throw_exception);
                }
            } else {
                if (itr.type == order_type::buy) {
                    assert(itr.price != 0 && itr.source_amount == 0);

                    auto find_itr = get_buy_itr(itr.create_time, u256(-1));
                    auto total_price = itr.price;
                    auto begin = find_itr.first;
                    auto end = find_itr.second;
                    if (begin != end) {

                        while (total_price > 0 && begin != end) {
                            auto order = (*begin).second.to_ex_order();
                            auto begin_total_price = order.token_amount * order.price;
                            result_order ret;
                            if (begin_total_price <= total_price) {
                                total_price -= begin_total_price;
                                ret.set_data(itr, &order, order.token_amount, order.price);
                                result.push_back(ret);
                                auto remove_id = order.trxid;
                                begin++;
                                if(!reset){
                                    //add_resultOrder(ret);
                                    remove_exchangeOrder((uint8_t)order.type, order.create_time, order.price, remove_id);
                                }

                            } else if (begin_total_price > total_price) {
                                auto can_buy_amount = total_price / order.price;
                                if (can_buy_amount == 0) {
                                    break;
                                }
                                ret.set_data(itr, &order, can_buy_amount, order.price);
                                result.push_back(ret);

                                if(!reset){
                                    //add_resultOrder(ret);
                                    //auto data_update = *begin;
                                    order.token_amount -= can_buy_amount;
                                    add_exchangeOrder(order);
                                }

                                break;
                            }
                        }



                    } else {
                        BOOST_THROW_EXCEPTION(all_price_operation_error());
                    }
                } else {   //all_price  , sell,
                    assert(itr.price == 0 && itr.source_amount != 0);

                    auto find_itr = get_sell_itr(itr.create_time, u256(0));
                    auto begin = find_itr.first;
                    auto end = find_itr.second;
                    auto total_amount = itr.token_amount;
                    if (begin != end) {
                        while (total_amount > 0 && begin != end) {
                            result_order ret;
                            ex_order order = (*begin).second.to_ex_order();
                            if ((*begin).second.m_pendingordertokenNum > total_amount) {
                                ret.set_data(itr, &order, total_amount, (*begin).second.m_pendingorderPrice);
                                result.push_back(ret);


                                if(!reset){
                                    auto data_update = (*begin).second.to_ex_order();
                                    data_update.token_amount -= total_amount;
                                    add_exchangeOrder(data_update);
                                }


                                total_amount = 0;
                            } else {
                                total_amount -= (*begin).second.m_pendingordertokenNum;
                                ret.set_data(itr, &order, (*begin).second.m_pendingordertokenNum, (*begin).second.m_pendingorderPrice);
                                result.push_back(ret);

                                auto remove_id = (*begin).second.m_orderId;
                                int64_t _time = (*begin).second.m_createTime;
                                uint8_t _type = (uint8_t)(*begin).second.m_pendingorderType;
                                u256 _price = (*begin).second.m_pendingorderPrice;
                                begin++;
                                if(!reset){
                                    remove_exchangeOrder(_type, _time, _price, remove_id);
                                }
                            }
                            //add_resultOrder(ret);
                        }
                    } else {
                        BOOST_THROW_EXCEPTION(all_price_operation_error());
                    }
                }
            }
            return result;
        }


        template<typename BEGIN, typename END>
        void newExdbState::process_only_price(BEGIN &begin, END &end, const ex_order &od, const u256 &price,
                                           const u256 &amount,
                                           std::vector<result_order> &result, bool throw_exception) {

            if (begin == end) {
                add_exchangeOrder(od);
                return;
            }
            auto spend = amount;

            bool rm = false;
            std::vector<std::tuple<uint8_t, int64_t, u256, h256>> removeHashs;
            while (spend > 0 && begin != end) {
                //ctrace << "spend  " << spend << " begin : " << begin->format_string();
                result_order ret;
                ex_order order = (*begin).second.to_ex_order();
                if ((*begin).second.m_pendingordertokenNum <= spend) {
                    spend -= (*begin).second.m_pendingordertokenNum;
                    ret.set_data(od, &order, (*begin).second.m_pendingordertokenNum, (*begin).second.m_pendingorderPrice);
                    rm = true;

                } else {
                    auto update = order;
                    update.token_amount -= spend;

                    //update data.
                    add_exchangeOrder(update);

                    ret.set_data(od, &order, spend, (*begin).second.m_pendingorderPrice);
                    spend = 0;
                }
                ret.old_price = price;

                //add_resultOrder(ret);

                result.push_back(ret);

                auto removeId = (*begin).second.m_orderId;
                int64_t _time = (*begin).second.m_createTime;
                uint8_t _type = (uint8_t)(*begin).second.m_pendingorderType;
                u256 _price = (*begin).second.m_pendingorderPrice;
                begin++;
                if (rm) {
                    removeHashs.push_back(std::make_tuple(_type, _time, _price, removeId));
                    rm = false;
                }
            }

            for(auto &itr : removeHashs){
                remove_exchangeOrder(std::get<0>(itr), std::get<1>(itr), std::get<2>(itr), std::get<3>(itr));
            }
            //surplus token ,  record to db
            if (spend > 0) {
                ex_order exod(od);
                exod.token_amount = spend;
                cwarn << "spend > 0  " << exod.format_string();
                add_exchangeOrder(exod);
            }
        }


        ex::order newExdbState::cancel_order_by_trxid(const dev::h256 &t) {
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


            remove_exchangeOrder((uint8_t)begin->type, begin->create_time, begin->price, begin->trxid);

            return o;
        }

        void newExdbState::add_exchangeOrder(const ex_order &od) {
            cdebug << "will  add_exchangeOrder txid " << od.format_string();
            //m_state.addExchangeOrder(ExdbSystemAddress, od);
            m_state.newAddExchangeOrder(ExdbSystemAddress, od);
        }


        void newExdbState::remove_exchangeOrder(uint8_t const& _orderType, int64_t const& _time, u256 const& _price,const h256 &id) {
            cdebug << "will remove txid " << toHex(id);
            //m_state.removeExchangeOrder(ExdbSystemAddress, id);
            m_state.newRemoveExchangeOrder(_orderType, _time, _price, id);
        }

        void newExdbState::add_resultOrder(const dev::brc::result_order &od) {
            //cdebug << "will add_resultOrder " << od.format_string();
           // m_state.addSuccessExchange(od);
        }

        std::vector<exchange_order> newExdbState::get_order_by_address(const Address &addr, uint32_t size) const {

            std::vector<exchange_order> ret;

            const auto &index = m_state.getExOrder().get<ex_by_address>();
            auto lower_itr = index.lower_bound(boost::tuple<Address, Time_ms>(addr, INT64_MAX));
            auto up_itr = index.upper_bound(boost::tuple<Address, Time_ms>(addr, 0));
            while (lower_itr != up_itr && lower_itr != index.end() && --size > 0) {
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

        std::vector<result_order> newExdbState::get_result_orders_by_news(uint32_t size) const {
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
        newExdbState::get_result_orders_by_address(const Address &addr, int64_t min_time, int64_t max_time,
                                                uint32_t max_size) const {
            std::vector<result_order> ret;
            std::vector<result_order> ret_sender;
            std::vector<result_order> ret_acceptor;

            uint32_t size = max_size;
            {
                const auto &sender_index = m_state.getSuccessExchange().get<ex_by_sender>();
                auto sender_lower_itr = sender_index.lower_bound(
                        boost::tuple<Address, Time_ms>(addr, min_time));
                auto sender_up_itr = sender_index.upper_bound(boost::tuple<Address, Time_ms>(addr, max_time));
                while (sender_lower_itr != sender_up_itr && size-- > 0) {
                    result_order eo;
                    eo.sender = sender_lower_itr->sender;
                    eo.acceptor = sender_lower_itr->acceptor;
                    eo.type = sender_lower_itr->type;
                    eo.token_type = sender_lower_itr->token_type;
                    eo.buy_type = sender_lower_itr->buy_type;
                    eo.create_time = sender_lower_itr->create_time;
                    eo.send_trxid = sender_lower_itr->send_trxid;
                    eo.to_trxid = sender_lower_itr->to_trxid;
                    eo.amount = sender_lower_itr->amount;
                    eo.price = sender_lower_itr->price;

                    ret_sender.push_back(eo);
                    sender_lower_itr++;
                }
            }

            {
                const auto &acceptor_index = m_state.getSuccessExchange().get<ex_by_acceptor>();
                auto acceptor_lower_itr = acceptor_index.lower_bound(
                        boost::tuple<Address, Time_ms>(addr, min_time));
                auto acceptor_up_itr = acceptor_index.upper_bound(
                        boost::tuple<Address, Time_ms>(addr, max_time));

                size = max_size;
                while (acceptor_lower_itr != acceptor_up_itr && size-- > 0) {
                    result_order eo;
                    eo.sender = acceptor_lower_itr->sender;
                    eo.acceptor = acceptor_lower_itr->acceptor;
                    eo.type = acceptor_lower_itr->type;
                    eo.token_type = acceptor_lower_itr->token_type;
                    eo.buy_type = acceptor_lower_itr->buy_type;
                    eo.create_time = acceptor_lower_itr->create_time;
                    eo.send_trxid = acceptor_lower_itr->send_trxid;
                    eo.to_trxid = acceptor_lower_itr->to_trxid;
                    eo.amount = acceptor_lower_itr->amount;
                    eo.price = acceptor_lower_itr->price;

                    ret_acceptor.push_back(eo);
                    acceptor_lower_itr++;
                }

            }


            //sort
            {
                size = 0;
                auto sender_itr = ret_sender.begin();
                auto acceptor_itr = ret_acceptor.begin();
                for (; size < max_size &&
                       (sender_itr != ret_sender.end() || acceptor_itr != ret_acceptor.end());
                       size++) {
                    if (sender_itr != ret_sender.end() && acceptor_itr != ret_acceptor.end()) {
                        if (sender_itr->create_time > acceptor_itr->create_time) {
                            ret.push_back(*sender_itr);
                            sender_itr++;
                        } else {
                            ret.push_back(*acceptor_itr);
                            acceptor_itr++;
                        }
                    } else if (sender_itr != ret_sender.end()) {
                        ret.push_back(*sender_itr);
                        sender_itr++;
                    } else {
                        ret.push_back(*acceptor_itr);
                        acceptor_itr++;
                    }
                }
            }
            return ret;
        }

        std::vector<exchange_order>
        newExdbState::get_order_by_type(order_type type, int64_t const& _time, u256 const& _price, uint32_t size) const {
            std::vector<exchange_order> ret;
            if (type == order_type::buy) {
                // const auto &index_greater = m_state.getExOrder().get<ex_by_price_greater>();
                // auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::buy,
                //                                                                             token_type,
                //                                                                             u256(-1), 0);
                // auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::buy,
                //                                                                             token_type,
                //                                                                             u256(0), INT64_MAX);
                // auto begin = index_greater.lower_bound(find_lower);
                // auto end = index_greater.upper_bound(find_upper);

                // while (begin != end && size > 0) {
                //     ret.push_back(exchange_order(*begin));
                //     begin++;
                //     size--;
                // }
                auto find_it = m_state.newGetBuyExchangeOrder(_time, _price);
                auto begin = find_it.first;
                auto end = find_it.second;
                while(begin != end && size > 0)
                {
                    ret.push_back((*begin).second.toExchangeOrder());
                    begin++;
                    size--;
                }
            } else {
                // const auto &index_less = m_state.getExOrder().get<ex_by_price_less>();
                // auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::sell,
                //                                                                             token_type,
                //                                                                             u256(0), 0);
                // auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::sell,
                //                                                                             token_type,
                //                                                                             u256(-1), INT64_MAX);
                // auto begin = index_less.lower_bound(find_lower);
                // auto end = index_less.upper_bound(find_upper);
                // while (begin != end && size > 0) {
                //     ret.push_back(exchange_order(*begin));
                //     begin++;
                //     size--;
                // }
                auto find_it = m_state.newGetSellExChangeOrder(_time, _price);

                auto begin = find_it.first;
                auto end = find_it.second;
                while(begin != end && size > 0)
                {
                    ret.push_back((*begin).second.toExchangeOrder());
                    begin++;
                    size--;
                }
            }
            return ret;
        }
//
        std::vector<ex_order> newExdbState::exits_trxid(const h256 &t) {

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