#pragma once


#include <brc/database.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <brc/exception.hpp>
#include <brc/types.hpp>

namespace dev {
    namespace brc {
        namespace ex {

            class exchange_plugin {
            public:
                exchange_plugin() : db(nullptr) {

                }

                ~exchange_plugin();
                exchange_plugin(const boost::filesystem::path &data_dir);


                exchange_plugin(const exchange_plugin &) = default;
                exchange_plugin(exchange_plugin&&) = default;
                exchange_plugin& operator=(exchange_plugin&&) = default;
                exchange_plugin& operator=(const exchange_plugin&) = default;


                /// insert operation .
                /// \param orders   vector<order>
                /// \param reset    if true , this operation will reset, dont record.
                /// \param throw_exception  if true  throw exception when error.
                /// \return
                std::vector<result_order> insert_operation(const std::vector<order> &orders, bool reset = true, bool throw_exception = false) ;

                /// get exchange order by address,
                /// \param addr   Address
                /// \return         complete order.
                std::vector<exchange_order> get_order_by_address(const Address &addr);

                /// get current all orders on exchange by size. this search by id in function.
                /// \param size  once get once.
                /// \return
                std::vector<exchange_order> get_orders(uint32_t size = 50) const;

                /// get newest result_order by size
                /// \param size         once get size.
                /// \return             vector<result_orders>.
                std::vector<result_order> get_result_orders_by_news(uint32_t size = 50);

                /// get exchange order by type (sell or buy && BRC or  FUEL)
                /// \param type         sell or buy
                /// \param token_type   BRC OR FUEL
                /// \param size         once search size.
                /// \return             complete order.
                std::vector<exchange_order> get_order_by_type(order_type type, order_token_type token_type, uint32_t size);

                /// rollback before packed block.
                /// \return
                bool rollback();


                ///  commit this state by block number.
                /// \param version  block number
                /// \return  true
                bool commit(int64_t version);

                ///
                /// \param os vector transactions id
                /// \param reset   if true, this operation rollback
                /// \return        vector<orders>
                std::vector<order>  cancel_order_by_trxid(const std::vector<h256> &os, bool reset);



            private:

                /// get iterator by type and price . this only find order of buy.
                /// \param token_type   BRC OR FUEL
                /// \param price        upper price.
                /// \return         std::pair<lower iterator, upper iterator>
                auto get_buy_itr(order_token_type token_type, u256 price) {
                    auto find_token = token_type == BRC ? FUEL : BRC;
                    const auto &index_greater = db->get_index<order_object_index>().indices().get<by_price_less>();

                    auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(sell, find_token,
                                                                                                u256(0), 0);
                    auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(sell, find_token,
                                                                                                price, INT64_MAX);

                    typedef decltype(index_greater.lower_bound(find_lower)) Lower_Type;
                    typedef decltype(index_greater.upper_bound(find_upper)) Upper_Type;

                    return std::pair<Lower_Type, Upper_Type>(index_greater.lower_bound(find_lower),
                                                             index_greater.upper_bound(find_upper));
                };

                /// get iterator by type and price, this only find of sell
                /// \param token_type   BRC OR FUEL,
                /// \param price        lower price.
                /// \return             std::pair<lower iterator, upper iterator>
                auto get_sell_itr(order_token_type token_type, u256 price) {
                    auto find_token = token_type == BRC ? FUEL : BRC;
                    const auto &index_less = db->get_index<order_object_index>().indices().get<by_price_greater>();  //↑

                    auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(buy, find_token,
                                                                                                u256(-1), 0);
                    auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(buy, find_token,
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
                void process_only_price(BEGIN &begin, END &end, const order &od, const u256 &price, const u256 &amount,
                              std::vector<result_order> &result, bool throw_exception) {
                    if (begin == end) {
                        db->create<order_object>([&](order_object &obj) {
                            obj.set_data(od, std::pair<u256, u256>(price, amount), amount);
                        });
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
                            ret.set_data(od, begin, amount, begin->price);
                            spend = 0;
                        }

                        db->create<order_result_object>([&](order_result_object &obj) {
                            obj.set_data(ret);
                        });
                        result.push_back(ret);
                        if (rm) {
                            const auto rm_obj = db->find(begin->id);
                            if (rm_obj != nullptr) {
                                begin++;
                                db->remove(*rm_obj);

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
                    }


                }


            //--------------------- members ---------------------
                /// database
                std::shared_ptr<database> db;
            };


        }
    }
}
