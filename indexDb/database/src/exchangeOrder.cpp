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
                    : db(new database(data_dir, chainbase::database::read_write, 1024 * 1024 * 1024ULL)),
                      _new_session(false) {

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
                    for (const auto &itr : orders) {
                        if (itr.buy_type == order_buy_type::only_price) {
                            if (itr.type == order_type::buy) {
                                auto find_itr = get_buy_itr(itr.token_type, itr.price_token.first);
                                process_only_price(find_itr.first, find_itr.second, itr, itr.price_token.first,
                                                   itr.price_token.second,
                                                   result,
                                                   throw_exception);

                            } else { //sell
                                auto find_itr = get_sell_itr(itr.token_type, itr.price_token.first);
                                process_only_price(find_itr.first, find_itr.second, itr, itr.price_token.first,
                                                   itr.price_token.second,
                                                   result,
                                                   throw_exception);
                            }

                        } else {
                            if (itr.type == order_type::buy) {
                                assert(itr.price_token.first != 0 && itr.price_token.second == 0);

                                auto find_itr = get_buy_itr(itr.token_type, u256(-1));
                                auto total_price = itr.price_token.first;
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

                                            db->create<order_result_object>([&](order_result_object &obj) {
                                                obj.set_data(ret);
                                            });
                                            update_dynamic_result_orders();

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

                                            db->create<order_result_object>([&](order_result_object &obj) {
                                                obj.set_data(ret);
                                            });
                                            update_dynamic_result_orders();

                                            const auto rm_obj = db->find(begin->id);
                                            db->modify(*rm_obj, [&](order_object &obj) {
                                                obj.token_amount -= can_buy_amount;
                                            });
                                            break;
                                        }
                                    }
                                } else {
                                    BOOST_THROW_EXCEPTION(all_price_operation_error());
                                }
                            } else {   //all_price  , sell,
                                assert(itr.price_token.first == 0 && itr.price_token.second != 0);
                                auto find_itr = get_sell_itr(itr.token_type, u256(0));
                                auto begin = find_itr.first;
                                auto end = find_itr.second;
                                auto total_amount = itr.price_token.second;
                                if (begin != end) {
                                    while (total_amount > 0 && begin != end) {
                                        result_order ret;
                                        if (begin->token_amount > total_amount) {
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
                db->with_write_lock([&]() {
                    check_db();


                    db->undo();

                    auto session = db->start_undo_session(true);
                    const auto &obj1 = db->get<dynamic_object>();
                    auto version = obj1.version;
                    db->modify(obj1, [version](dynamic_object &obj) {
                        obj.version = version + 1;
                    });
                    session.push();
                });
                return true;
            }

            bool exchange_plugin::rollback_until(const h256 &block_hash, const h256 &root_hash) {
                return db->with_write_lock([&]() -> bool {
                    uint32_t maxCount = 12;
                    while (maxCount-- > 0) {
                        const auto &obj = db->get<dynamic_object>();
                        db->undo();
//                    cwarn << "undo  " << check_version(false);
                        if (obj.block_hash == block_hash && obj.root_hash == root_hash) {
                            return true;
                        }
                    }
                    return false;
                });

            }

            void exchange_plugin::new_session(int64_t version, const dev::h256 &block_hash,
                                              const dev::h256 &root_hash) {
                if (!_new_session) {

                    db->with_write_lock([&]() {
                        auto session = db->start_undo_session(true);
                        const auto &obj = db->get<dynamic_object>();
                        db->modify(obj, [&](dynamic_object &obj) {
                            obj.version = version;
                            obj.block_hash = block_hash;
                            obj.root_hash = root_hash;
                        });
                        session.push();

                        _new_session = true;
                    });


                }
            }

            bool exchange_plugin::commit(int64_t version, const h256 &block_hash, const h256 &root_hash) {
                check_db();

                db->with_write_lock([&]() {
                    const auto &obj = db->get<dynamic_object>();
                    db->modify(obj, [&](dynamic_object &obj) {
                        obj.version = version;
                        obj.block_hash = block_hash;
                        obj.root_hash = root_hash;
                    });

                    auto session = db->start_undo_session(true);
                    const auto &obj1 = db->get<dynamic_object>();
                    db->modify(obj1, [&](dynamic_object &obj) {
                        obj.version = version + 1;
                    });
                    session.push();
                });

                return true;
            }

            bool exchange_plugin::commit_disk(int64_t version, bool first_commit) {
                if (first_commit) {
                    const auto &obj = db->get<dynamic_object>();
                    db->modify(obj, [&](dynamic_object &obj) {
                        obj.version = version;
                    });
                }
                db->commit(version);
                db->flush();
                return true;
            }

            bool exchange_plugin::remove_all_session() {
                db->undo_all();
                return true;
            }

            std::vector<exchange_order>
            exchange_plugin::get_order_by_type(order_type type, order_token_type token_type, uint32_t size) const {
                check_db();
                vector<exchange_order> ret;
                if (type == order_type::buy) {
                    const auto &index_greater = db->get_index<order_object_index>().indices().get<by_price_greater>();
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
                    const auto &index_less = db->get_index<order_object_index>().indices().get<by_price_less>();
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

            std::vector<order> exchange_plugin::cancel_order_by_trxid(const std::vector<h256> &os, bool reset) {
                check_db();
                auto session = db->start_undo_session(true);
                std::vector<order> ret;
                const auto &index_trx = db->get_index<order_object_index>().indices().get<by_trx_id>();
                for (const auto &t : os) {
                    auto begin = index_trx.lower_bound(t);
                    auto end = index_trx.upper_bound(t);
                    if (begin == end) {
                        BOOST_THROW_EXCEPTION(find_order_trxid_error() << errinfo_comment(toString(t)));
                    }
                    while (begin != end) {
                        order o;
                        o.trxid = begin->trxid;
                        o.sender = begin->sender;
                        o.buy_type = order_buy_type::only_price;
                        o.token_type = begin->token_type;
                        o.type = begin->type;
                        o.time = begin->create_time;
                        //                        o.price_token[begin->price] = begin->token_amount;
                        o.price_token.first = begin->price;
                        o.price_token.second = begin->token_amount;

                        ret.push_back(o);

                        const auto rm = db->find(begin->id);
                        begin++;
                        db->remove(*rm);
                    }
                    update_dynamic_orders(false);
                }
                if (!reset) {
                    session.squash();
                }
                return ret;
            }

            const dynamic_object &exchange_plugin::get_dynamic_object() const {
                return db->get<dynamic_object>();
            }

            void exchange_plugin::update_dynamic_orders(bool up) {
                db->modify(get_dynamic_object(), [&](dynamic_object &obj) {
                    if (up) {
                        obj.orders++;
                    } else {
                        obj.orders--;
                    }
                });
            }

            void exchange_plugin::update_dynamic_result_orders() {
                db->modify(get_dynamic_object(), [&](dynamic_object &obj) {
                    obj.result_orders++;
                });
            }

            template<typename BEGIN, typename END>
            void exchange_plugin::process_only_price(BEGIN &begin, END &end, const order &od, const u256 &price,
                                                     const u256 &amount,
                                                     std::vector<result_order> &result, bool throw_exception) {
                if (begin == end) {
                    db->create<order_object>([&](order_object &obj) {
                        obj.set_data(od, std::pair<u256, u256>(price, amount), amount);
                    });

                    update_dynamic_orders(true);
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
                        db->modify(*begin, [&](order_object &obj) {
                            obj.token_amount -= spend;
                        });
                        ret.set_data(od, begin, spend, begin->price);
                        spend = 0;
                    }
                    ret.old_price = price;

                    db->create<order_result_object>([&](order_result_object &obj) {
                        obj.set_data(ret);
                    });
                    update_dynamic_result_orders();

                    result.push_back(ret);
                    if (rm) {
                        const auto rm_obj = db->find(begin->id);
                        if (rm_obj != nullptr) {
                            begin++;
                            db->remove(*rm_obj);
                            update_dynamic_orders(false);
                        } else {
                            if (throw_exception) {
                                BOOST_THROW_EXCEPTION(remove_object_error());
                            }
                        }
                        rm = false;
                    } else {
                        begin++;
                    }

                }
                //surplus token ,  record to db
                if (spend > 0) {
                    db->create<order_object>([&](order_object &obj) {
                        obj.set_data(od, std::pair<u256, u256>(price, amount), spend);
                    });
                    update_dynamic_orders(true);
                }
            }


            std::vector<result_order> exchange_plugin::get_result_orders_by_address(const dev::Address &addr,
                                                                                    int64_t min_time, int64_t max_time,
                                                                                    uint32_t max_size) const {
                if (max_size > 50 || min_time >= max_time) {
                    BOOST_THROW_EXCEPTION(get_result_order_error());
                }

                return db->with_read_lock([&]() {
                    std::vector<result_order> ret;
                    std::vector<result_order> ret_sender;
                    std::vector<result_order> ret_acceptor;

                    uint32_t size = max_size;
                    {
                        const auto &sender_index = db->get_index<order_result_object_index>().indices().get<by_sender>();
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
                        const auto &acceptor_index = db->get_index<order_result_object_index>().indices().get<by_acceptor>();
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
                });
            }


        }
    }
}
