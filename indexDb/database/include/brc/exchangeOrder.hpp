#pragma once


#include <brc/database.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <brc/exception.hpp>
#include <brc/types.hpp>
namespace brc{

    using namespace db;


    namespace ex{

        class exchange_plugin{
        public:
            exchange_plugin(const boost::filesystem::path &data_dir);
            std::vector<result_order> insert_operation(const std::vector<order> &orders, bool reset = true, bool throw_exception = false);

            std::vector<exchange_order>  get_order_by_address(const Address &addr);
            std::vector<exchange_order>  get_oeders();

//            int64_t     get_version();
//            bool        rollback(int version);

        private:


            template <typename BEGIN, typename END>
            void proccess(BEGIN &begin, END &end, const order &od, const u256 &price, const u256 &amount, std::vector<result_order> &result, bool throw_exception){
                if(begin == end){
                    //TODO: create order_object
                    db.create<order_object>([&](order_object &obj) {
                        obj.set_data(od, std::pair<u256, u256>(price, amount), amount);
                    });
                    return ;
                }
                auto spend = amount;

                while(spend > 0  && begin != end){
                    std::cout << "spend: " << begin->token_amount << std::endl;
                    result_order ret;
                    if(begin->token_amount <= spend){
                        spend -= begin->token_amount;

                        ret.set_data(od, begin, begin->token_amount, price);
//                        db.remove(*begin);
                    } else{
                        db.modify(*begin, [&](order_object &obj){
                            obj.token_amount -= spend;
                        });
                        ret.set_data(od, begin, amount, price);
                        spend = 0;
                    }

                    db.create<order_result_object>([&](order_result_object &obj) {
                        obj.set_data(ret);
                    });
                    result.push_back(ret);
                    begin++;
                }

                //surplus token ,  record to db
                if(spend > 0){
                    db.create<order_object>([&](order_object &obj) {
                        obj.set_data(od, std::pair<u256, u256>(price, amount), spend);
                    });
                }


            }






            brc::db::database db;
        };


    }
}