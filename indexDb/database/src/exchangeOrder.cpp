#include <brc/exchangeOrder.hpp>



#include <boost/tuple/tuple.hpp>
#include <brc/exception.hpp>
#include <libbrccore/CommonJS.h>
#include <vector>


using namespace dev;
using namespace std;
namespace dev {
    namespace brc {
        namespace ex {

            exchange_plugin::exchange_plugin(const boost::filesystem::path &data_dir)
                    : db(new database(data_dir, chainbase::database::read_write, 1024 * 1024 * 1024ULL)) {

                db->add_index<order_object_index>();
                db->add_index<order_result_object_index>();
                db->add_index<dynamic_object_index>();


                if (!db->find<dynamic_object>()) {
                    db->create<dynamic_object>([](dynamic_object &obj) {
                    });
                }
            }

            exchange_plugin::~exchange_plugin() {

            }

            std::vector<result_order>
            exchange_plugin::insert_operation(const std::vector<order> &orders, bool reset, bool throw_exception) {
                return db->with_write_lock([&]() {

                    check_db();
                    auto session = db->start_undo_session(true);
                    std::vector<result_order> result;
//                    try {
                        for (const auto &itr : orders) {
                            if (itr.buy_type == order_buy_type::only_price) {
                                for (const auto t :  itr.price_token) {
                                    if (itr.type == order_type::buy) {
                                        auto find_itr = get_buy_itr(itr.token_type, t.first);
                                        process_only_price(find_itr.first, find_itr.second, itr, t.first, t.second,
                                                           result,
                                                           throw_exception);
                                    } else { //sell
                                        auto find_itr = get_sell_itr(itr.token_type, t.first);
                                        process_only_price(find_itr.first, find_itr.second, itr, t.first, t.second,
                                                           result,
                                                           throw_exception);
                                    }

                                }
                            } else {
                                if (itr.price_token.size() != 1) {
                                    BOOST_THROW_EXCEPTION(all_price_operation_error());
                                }
                                if (itr.type == order_type::buy) {
                                    auto find_itr = get_buy_itr(itr.token_type, u256(-1));
                                    auto total_price = itr.price_token.begin()->first;
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
                                                const auto rm_obj = db->find(begin->id);
                                                begin++;
                                                db->remove(*rm_obj);
                                                update_dynamic_orders(false);
                                            } else if (begin_total_price > total_price) {

                                                auto can_buy_amount = total_price / begin->price;
                                                if (can_buy_amount == 0) {
                                                    break;
                                                }
                                                ret.set_data(itr, begin, can_buy_amount, begin->price);
                                                result.push_back(ret);
                                                const auto rm_obj = db->find(begin->id);
                                                db->modify(*rm_obj, [&](order_object &obj) {
                                                    obj.token_amount -= can_buy_amount;
                                                });
                                                begin++;
                                            }

                                            db->create<order_result_object>([&](order_result_object &obj) {
                                                obj.set_data(ret);
                                            });
                                            update_dynamic_result_orders();
                                        }
                                    } else {
                                        BOOST_THROW_EXCEPTION(all_price_operation_error());
                                    }
                                } else {   //all_price  , sell,
                                    auto find_itr = get_sell_itr(itr.token_type, u256(0));
                                    auto begin = find_itr.first;
                                    auto end = find_itr.second;
                                    auto total_amount = itr.price_token.begin()->second;
                                    if (begin != end) {
                                        while (total_amount > 0 && begin != end) {
                                            result_order ret;
                                            if (begin->token_amount >= total_amount) {
                                                ret.set_data(itr, begin, total_amount, begin->price);
                                                result.push_back(ret);
                                                const auto rm_obj = db->find(begin->id);
                                                db->modify(*rm_obj, [&](order_object &obj) {
                                                    obj.token_amount -= total_amount;
                                                });
                                                total_amount = 0;
                                            } else {
                                                total_amount -= begin->token_amount;
                                                ret.set_data(itr, begin, begin->token_amount, begin->price);
                                                result.push_back(ret);
                                                const auto rm_obj = db->find(begin->id);
                                                begin++;
                                                db->remove(*rm_obj);
                                                update_dynamic_orders(false);
                                            }
                                            db->create<order_result_object>([&](order_result_object &obj) {
                                                obj.set_data(ret);
                                            });
                                            update_dynamic_result_orders();
                                        }
                                    } else {
                                        BOOST_THROW_EXCEPTION(all_price_operation_error());
                                    }
                                }


                            }

                        }

                        if (!reset) {
                            session.squash();
                        }
//                    } catch (const dev::Exception &e) {
//                        session.undo();
//                        BOOST_THROW_EXCEPTION(e);
//                    } catch (const std::exception &e) {
//                        session.undo();
//                        BOOST_THROW_EXCEPTION(e);
//                    }catch (...){
//                        session.undo();
//                        BOOST_THROW_EXCEPTION(createOrderError());
//                    }

                    return result;
                });
            }

            std::vector<exchange_order> exchange_plugin::get_order_by_address(const Address &addr) const {
                check_db();
                std::vector<exchange_order> ret;

                const auto &index = db->get_index<order_object_index>().indices().get<by_address>();
                auto lower_itr = index.lower_bound(boost::tuple<Address, Time_ms>(addr, INT64_MAX));
                auto up_itr = index.upper_bound(boost::tuple<Address, Time_ms>(addr, 0));
                while (lower_itr != up_itr && lower_itr != index.end()) {
                    ret.push_back(exchange_order(*lower_itr));
                    lower_itr++;
                }

                return ret;
            }

            std::vector<exchange_order> exchange_plugin::get_orders(uint32_t size) const {
                check_db();
                std::vector<exchange_order> ret;
                const auto &index = db->get_index<order_object_index>().indices().get<by_price_less>();
                auto begin = index.begin();
                while (begin != index.end() && size > 0) {
                    ret.push_back(exchange_order(*begin));
                    begin++;
                    size--;
                }
                return ret;
            }

            std::vector<result_order> exchange_plugin::get_result_orders_by_news(uint32_t size) const {
                check_db();
                vector<result_order> ret;
                const auto &index = db->get_index<order_result_object_index>().indices().get<by_greater_id>();
                auto begin = index.begin();
                while (begin != index.end() && size > 0) {
                    result_order eo;

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


            bool exchange_plugin::rollback() {
                check_db();
                db->undo();
                return true;
            }

            bool exchange_plugin::commit(int64_t version) {
                check_db();
                const auto &obj = db->get<dynamic_object>();
                db->modify(obj, [&](dynamic_object &obj) {
                    obj.version = version;
                });
                db->commit(version);
                db->flush();
//                cwarn << "commit rollback version  exchange database version : " << obj.version << " orders: " << obj.orders << " ret_orders:" << obj.result_orders;
                return true;
            }


            std::vector<exchange_order>
            exchange_plugin::get_order_by_type(order_type type, order_token_type token_type, uint32_t size) const {
                check_db();
                vector<exchange_order> ret;
                if (type == order_type::buy) {
                    const auto &index_greater = db->get_index<order_object_index>().indices().get<by_price_greater>();
                    auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::buy, token_type,
                                                                                                u256(-1), 0);
                    auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::buy, token_type,
                                                                                                u256(0), INT64_MAX);
                    auto begin = index_greater.lower_bound(find_lower);
                    auto end = index_greater.upper_bound(find_upper);

                    while (begin != end && size > 0) {
                        ret.push_back(exchange_order(*begin));
                        begin++;
                        size--;
                    }
                } else {
                    const auto &index_less = db->get_index<order_object_index>().indices().get<by_price_less>();
                    auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::sell, token_type,
                                                                                                u256(0), 0);
                    auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(order_type::sell, token_type,
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

            std::vector<order> exchange_plugin::cancel_order_by_trxid(const std::vector<h256> &os, bool reset) {
                check_db();
                auto session = db->start_undo_session(true);
                std::vector<order> ret;
                const auto &index_trx = db->get_index<order_object_index>().indices().get<by_trx_id>();
                for (const auto &t : os) {
                    auto begin = index_trx.lower_bound(t);
                    auto end = index_trx.upper_bound(t);
                    if (begin == end) {
                        BOOST_THROW_EXCEPTION(find_order_trxid_error());
                    }
                    order o;
                    o.trxid = begin->trxid;
                    o.sender = begin->sender;
                    o.buy_type = order_buy_type::only_price;
                    o.token_type = begin->token_type;
                    o.type = begin->type;
                    o.time = begin->create_time;
                    while (begin != end) {
                        o.price_token[begin->price] = begin->token_amount;
                        const auto rm = db->find(begin->id);
                        db->remove(*rm);
                        begin++;
                    }
                    ret.push_back(o);
                }
                if (!reset) {
                    session.push();
                }
                return ret;
            }

        }
    }
}
