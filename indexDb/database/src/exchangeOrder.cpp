#include <brc/exchangeOrder.hpp>
#include <boost/tuple/tuple.hpp>




//#define GET_ITR_BEGIN_LOWWER(INDEX_TYPE, INDEX, TYPE, PRICE) \
//     db.get_index<INDEX>().indices().get<INDEX>().lower_bound(boost::tuple<order_token_type, order_token_type>(2, BRC));

using namespace dev;

namespace brc {
    namespace ex {

        exchange_plugin::exchange_plugin(const boost::filesystem::path &data_dir)
                : db(data_dir, chainbase::database::read_write, 1024 * 64) {

        }

        std::vector<result_order>
        exchange_plugin::insert_operation(const std::vector<order> &orders, bool reset, bool throw_exception) {

            std::vector<result_order> result;
            auto session = db.start_undo_session(true);
            const auto &index = db.get_index<order_object_index>().indices().get<by_price_buy>();
            try {
                for (const auto &itr : orders) {
                    if (itr.buy_type == only_price) {
                        for (const auto t :  itr.price_token) {
                            order_type o_type = (order_type) (1 ^ itr.type);
                            order_token_type t_type = (order_token_type) (1 ^ itr.token_type);
                            auto find_lower_type = boost::tuple<order_type, order_token_type, u256, uint64_t>(o_type,
                                                                                                              t_type,
                                                                                                              u256(0),
                                                                                                              UINT64_MAX);
                            auto find_upper_type = boost::tuple<order_type, order_token_type, u256, uint64_t>(o_type,
                                                                                                              t_type,
                                                                                                              t.first,
                                                                                                              UINT64_MAX);



                            auto begin = index.lower_bound(find_lower_type);
                            auto end = index.upper_bound(find_upper_type);
                            if (begin == end) {
                                db.create<order_object>([&](order_object &obj) {
                                    obj.set_data(itr, t, t.second);
                                });
                            } else {
                                auto amount = t.second;
                                while (amount > 0) {
                                    if (begin == end) {
                                        //left token  push order_object
                                        if (amount != 0) {
                                            db.create<order_object>([&](order_object &obj) {
                                                obj.set_data(itr, t, amount);
                                            });
                                        }
                                    }
                                    if (begin->token_amount <= amount) {
                                        amount -= begin->token_amount;
                                        result_order ret(itr, begin, begin->token_amount, t.first);
                                        db.create<order_result_object>([&](order_result_object &obj) {
                                            obj.set_data(ret);
                                        });
                                        db.remove(*begin);
                                        result.push_back(ret);
                                    } else {
                                        db.modify(*begin, [&](order_object &obj) {
                                            obj.token_amount -= amount;

                                        });
                                        result_order ret(itr, begin, amount, t.first);
                                        db.create<order_result_object>([&](order_result_object &obj) {
                                            obj.set_data(ret);
                                        });
                                        amount = 0;
                                    }
                                    begin++;
                                }

                            }
                        }
                    }
//                    else {  //all_price
//                        for(const auto t :  itr.price_token){
//                            order_type o_type = (order_type)(1 ^ itr.type);
//                            order_token_type t_type = (order_token_type)(1 ^ itr.token_type);
//                            auto find_lower_type = boost::tuple<order_type, order_token_type, u256, uint64_t>(o_type, t_type, u256(0), UINT64_MAX);
//                            auto find_upper_type = boost::tuple<order_type, order_token_type, u256, uint64_t>(o_type, t_type, t.first, UINT64_MAX);
//
//
//                        }
//                    }

                }

            } catch (const boost::exception &e) {
                return result;
            }
            return result;
        }

    }
}