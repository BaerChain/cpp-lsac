//
// Created by friday on 2019/8/19.
//

#include "ExdbState.h"


namespace dev {
    namespace brc {


        std::vector<ex::result_order> ExdbState::insert_operation(const ex::ex_order &itr) {

            std::vector<result_order> result;
            bool throw_exception = true;
            if (itr.buy_type == order_buy_type::only_price) {
                if (itr.type == order_type::buy) {
                    auto find_itr = get_buy_itr(itr.token_type, itr.price);
                    process_only_price(find_itr.first, find_itr.second, itr, itr.price,
                                       itr.source_amount,
                                       result,
                                       throw_exception);

                }
                else { //sell
                    auto find_itr = get_sell_itr(itr.token_type, itr.price);
                    process_only_price(find_itr.first, find_itr.second, itr, itr.price,
                                       itr.source_amount,
                                       result,
                                       throw_exception);
                }

            } else {
//                if (itr.type == order_type::buy) {
//                    assert(itr.source_amount != 0 && itr.price == 0);
//
//                    auto find_itr = get_buy_itr(itr.token_type, u256(-1));
//                    auto total_price = itr.price_token.first;
//                    auto begin = find_itr.first;
//                    auto end = find_itr.second;
//                    if (begin != end) {
//                        while (total_price > 0 && begin != end) {
//                            auto begin_total_price = begin->token_amount * begin->price;
//                            result_order ret;
//                            if (begin_total_price <= total_price) {   //
//                                total_price -= begin_total_price;
//                                ret.set_data(itr, begin, begin->token_amount, begin->price);
//                                result.push_back(ret);
//
//                                db->create<order_result_object>([&](order_result_object &obj) {
//                                    obj.set_data(ret);
//                                });
//                                update_dynamic_result_orders();
//
//                                const auto rm_obj = db->find(begin->id);
//                                begin++;
//                                db->remove(*rm_obj);
//                                update_dynamic_orders(false);
//                            } else if (begin_total_price > total_price) {
//                                auto can_buy_amount = total_price / begin->price;
//                                if (can_buy_amount == 0) {
//                                    break;
//                                }
//                                ret.set_data(itr, begin, can_buy_amount, begin->price);
//                                result.push_back(ret);
//
//                                db->create<order_result_object>([&](order_result_object &obj) {
//                                    obj.set_data(ret);
//                                });
//                                update_dynamic_result_orders();
//
//                                const auto rm_obj = db->find(begin->id);
//                                db->modify(*rm_obj, [&](order_object &obj) {
//                                    obj.token_amount -= can_buy_amount;
//                                });
//                                break;
//                            }
//                        }
//                    } else {
//                        BOOST_THROW_EXCEPTION(all_price_operation_error());
//                    }
//                }
//                else {   //all_price  , sell,
//                    assert(itr.price_token.first == 0 && itr.price_token.second != 0);
//                    auto find_itr = get_sell_itr(itr.token_type, u256(0));
//                    auto begin = find_itr.first;
//                    auto end = find_itr.second;
//                    auto total_amount = itr.price_token.second;
//                    if (begin != end) {
//                        while (total_amount > 0 && begin != end) {
//                            result_order ret;
//                            if (begin->token_amount > total_amount) {
//                                ret.set_data(itr, begin, total_amount, begin->price);
//                                result.push_back(ret);
//                                const auto rm_obj = db->find(begin->id);
//                                db->modify(*rm_obj, [&](order_object &obj) {
//                                    obj.token_amount -= total_amount;
//                                });
//                                total_amount = 0;
//                            } else {
//                                total_amount -= begin->token_amount;
//                                ret.set_data(itr, begin, begin->token_amount, begin->price);
//                                result.push_back(ret);
//                                const auto rm_obj = db->find(begin->id);
//                                begin++;
//                                db->remove(*rm_obj);
//                                update_dynamic_orders(false);
//                            }
//                            db->create<order_result_object>([&](order_result_object &obj) {
//                                obj.set_data(ret);
//                            });
//                            update_dynamic_result_orders();
//                        }
//                    } else {
//                        BOOST_THROW_EXCEPTION(all_price_operation_error());
//                    }
//                }
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

//                db->create<order_result_object>([&](order_result_object &obj) {
//                    obj.set_data(ret);
//                });
//                update_dynamic_result_orders();

                result.push_back(ret);
                if (rm) {
                    remove_exchangeOrder(begin->trxid);
                } else {
                    begin++;
                }

            }
            //surplus token ,  record to db
//            if (spend > 0) {
//                db->create<order_object>([&](order_object &obj) {
//                    obj.set_data(od, std::pair<u256, u256>(price, amount), spend);
//                });
//                update_dynamic_orders(true);
//            }
        }


        ex::order ExdbState::cancel_order_by_trxid(const dev::h256 &id) {


            return ex::order();
        }

        void ExdbState::add_exchangeOrder(const ex_order &od) {
            static Address systemADD;
            m_state.addExchangeOrder(systemADD, od);
        }


        void ExdbState::remove_exchangeOrder(const dev::h256 &id) {
            static Address systemADD;
            m_state.removeExchangeOrder(systemADD, id);
        }

    }
}