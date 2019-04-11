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

                exchange_plugin(const boost::filesystem::path &data_dir);

                exchange_plugin(exchange_plugin&&) = default;
                exchange_plugin& operator=(exchange_plugin&&) = default;



                std::vector<result_order>
                insert_operation(const std::vector<order> &orders, bool reset = true, bool throw_exception = false);

                std::vector<exchange_order> get_order_by_address(const Address &addr);

                std::vector<exchange_order> get_orders();

                bool rollback();

                bool commit(int64_t version);


            private:


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
//                    std::cout << "begin time: " << begin->create_time << std::endl;
//                    std::cout << "begin token_amount: " << begin->token_amount << std::endl;
//                    std::cout << "begin price: " << begin->price << std::endl;
//                    std::cout << "price: " << price << std::endl;
//                    std::cout << "amount: " << amount << std::endl;

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


//                template <typename BEGIN, typename END>
//                void process_all_price(BEGIN &begin, END &end, const order &od, std::vector<result_order> &result, bool throw_exception)
//                {
//                    if(begin == end){
//                        BOOST_THROW_EXCEPTION(all_price_operation_error());
//                    }
//                    auto total_price = od.total_price;
//                    while(total_price > 0 && begin != end){
//                        auto sum = begin->token_amount * begin->price;
//                        if(sum >= total_price){
//                            auto s_amount = total_price / begin->price;
//                            auto s_price = s_amount * begin->price;
//
//                            assert(sum > s_price);
//
//                            result_order ret;
//                            ret.set_data(od, begin, s_amount, begin->price);
//                            const auto rm_obj = db->find(begin->id);
//                            if(s_price == sum){ //remove
//                                db->remove(*rm_obj);
//                            }
//                            else{
//                                db->modify(*rm_obj, [&](order_object &obj){
//                                    obj.token_amount -= s_amount;
//                                });
//                            }
//
//
//                            result.push_back(ret);
//                            break;
//                        }
//                        else{ // this order of all_price can eat this order.  find next . record cost amount.
//                            auto s_amount = sum / begin->price;
//
//
//
//
//
//                        }
//                    }
//
//
//
//                }



                std::shared_ptr<database> db;
            };


        }
    }
}
