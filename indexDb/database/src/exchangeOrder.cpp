#include <brc/exchangeOrder.hpp>
#include <boost/tuple/tuple.hpp>




//#define GET_ITR_BEGIN_LOWWER(INDEX_TYPE, INDEX, TYPE, PRICE) \
//     db.get_index<INDEX>().indices().get<INDEX>().lower_bound(boost::tuple<order_token_type, order_token_type>(2, BRC));

#define IF_THROW_EXCEPTION(FLAG, EXP, RETURN) \
        if(FLAG) { EXP; }else{ return RETURN;}



using namespace dev;

namespace brc {
    namespace ex {

        exchange_plugin::exchange_plugin(const boost::filesystem::path &data_dir)
                : db(data_dir, chainbase::database::read_write, 1024 * 1024 * 1024ULL) {

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


                            u256 price_lower = 0;
                            u256 price_up = 0;

                            if(itr.type == buy){
                                price_up = t.first;
                            }
                            else if(itr.type == sell){
                                price_lower = t.first;
                                price_up = Invalid256;
                            }
                            else{
                                IF_THROW_EXCEPTION(throw_exception, BOOST_THROW_EXCEPTION(order_type_is_null()), result);
                            }

                            auto find_lower_type = boost::tuple<order_type, order_token_type, u256, uint64_t>(o_type,
                                                                                                              t_type,
                                                                                                              price_lower,
                                                                                                              UINT64_MAX);
                            auto find_upper_type = boost::tuple<order_type, order_token_type, u256, uint64_t>(o_type,
                                                                                                              t_type,
                                                                                                              price_up,
                                                                                                              UINT64_MAX);
                            auto lower_itr = index.lower_bound(find_lower_type);
                            auto up_itr = index.upper_bound(find_upper_type);


                            if (lower_itr == up_itr) {
                                db.create<order_object>([&](order_object &obj) {
                                    obj.set_data(itr, t, t.second);
                                });
                            } else {
                                auto amount = t.second;
                                auto begin = itr.type == buy ? lower_itr : up_itr;
                                auto end = itr.type == buy ? up_itr : lower_itr;
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
                                        std::cout << "expire : " << amount << std::endl;
                                        result_order ret(itr, begin, begin->token_amount, t.first);
                                        const auto *rm_item = db.find(begin->id);
                                        if(rm_item != nullptr){
                                            db.remove(*rm_item);
                                        }
                                        else{
                                            IF_THROW_EXCEPTION(throw_exception, BOOST_THROW_EXCEPTION(remove_object_error()), result);
                                        }
                                        db.create<order_result_object>([&](order_result_object &obj) {
                                            obj.set_data(ret);
                                        });
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
                                    itr.type == buy ? begin++ : begin--;
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

                auto version = session.revision();
                if(!reset){
                    db.commit(version + 1);
                }
            } catch (const boost::exception &e) {
                return result;
            }
            return result;
        }

        std::vector<exchange_order> exchange_plugin::get_order_by_address(const Address &addr) {
            std::vector<exchange_order> ret;

//            const auto &index = db.get_index<order_object_index>().indices().get<by_address>();
//            auto lower_itr = index.lower_bound(boost::tuple<shared_Address, Time_ms >(addr, INT64_MAX));
//            auto up_itr = index.upper_bound(boost::tuple<shared_Address, Time_ms>(addr, 0));
//            while(lower_itr != up_itr && lower_itr != index.end()){
//                ret.push_back(exchange_order(*lower_itr));
//                lower_itr++;
//            }

            return ret;
        }

        std::vector<exchange_order> exchange_plugin::get_oeders() {

            vector<exchange_order> ret;
            const auto &index = db.get_index<order_object_index>().indices().get<by_id>();
            auto begin = index.begin();
            while(begin != index.end()){
                ret.push_back(exchange_order(*begin));
                begin++;
            }

            return ret;
        }

    }
}