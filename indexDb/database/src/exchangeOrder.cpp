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
                : db(new database(data_dir, chainbase::database::read_write, 1024 * 1024 * 1024ULL)) {

        }

        std::vector<result_order>
        exchange_plugin::insert_operation(const std::vector<order> &orders, bool reset, bool throw_exception) {
            if(!db){
                BOOST_THROW_EXCEPTION(get_db_instance_error());
            }

            std::vector<result_order> result;
            auto session = db->start_undo_session(true);
            try {
                // get itr by type and token_type
                // @param less
                // @return pair,   first is begin iterator,  second is end iterator.
                auto get_buy_itr = [&](order_token_type token_type, u256 price){
                    auto find_token = token_type == BRC ? FUEL : BRC;
                    const auto &index_greater = db->get_index<order_object_index>().indices().get<by_price_buy_less>();

                    auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms >(sell, find_token, u256(0), 0);
                    auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(sell, find_token, price, INT64_MAX);

                    typedef decltype(index_greater.lower_bound(find_lower)) Lower_Type;
                    typedef decltype(index_greater.upper_bound(find_upper)) Upper_Type;

                    return std::pair<Lower_Type, Upper_Type>(index_greater.lower_bound(find_lower), index_greater.upper_bound(find_upper));
                };

                auto get_sell_itr = [&](order_token_type token_type, u256 price){
                    auto find_token = token_type == BRC ? FUEL : BRC;
                    const auto &index_less = db->get_index<order_object_index>().indices().get<by_price_buy_greater>();  //↑

                    auto find_lower = boost::tuple<order_type, order_token_type, u256, Time_ms>(buy, find_token, u256(-1), 0);
                    auto find_upper = boost::tuple<order_type, order_token_type, u256, Time_ms>(buy, find_token, price, INT64_MAX);

                    typedef decltype(index_less.lower_bound(find_lower)) Lower_Type;
                    typedef decltype(index_less.upper_bound(find_upper)) Upper_Type;

                    return std::pair<Lower_Type, Upper_Type>(index_less.lower_bound(find_lower), index_less.upper_bound(find_upper));
                };

                for (const auto &itr : orders) {
                    if (itr.buy_type == only_price) {
                        for (const auto t :  itr.price_token) {
                           if(itr.type == buy){
                               auto find_itr = get_buy_itr(itr.token_type, t.first);
                               proccess(find_itr.first, find_itr.second, itr, t.first, t.second, result, throw_exception);
                           }
                           else{ //sell
                               auto find_itr = get_sell_itr(itr.token_type, t.first);
                               proccess(find_itr.first, find_itr.second, itr, t.first, t.second, result, throw_exception);
                           }

                        }
                    }

                }



                if(!reset){
                    auto version = db->revision();
                    db->commit(version + 1);

                }
            } catch (const boost::exception &e) {
                return result;
            }
            return result;
        }

        std::vector<exchange_order> exchange_plugin::get_order_by_address(const Address &addr) {
            std::vector<exchange_order> ret;

            const auto &index = db->get_index<order_object_index>().indices().get<by_address>();
            auto lower_itr = index.lower_bound(boost::tuple<Address, Time_ms >(addr, INT64_MAX));
            auto up_itr = index.upper_bound(boost::tuple<Address, Time_ms>(addr, 0));
            while(lower_itr != up_itr && lower_itr != index.end()){
                ret.push_back(exchange_order(*lower_itr));
                lower_itr++;
            }

            return ret;
        }

        std::vector<exchange_order> exchange_plugin::get_orders() {

            vector<exchange_order> ret;
            const auto &index = db->get_index<order_object_index>().indices().get<by_price_buy_less>();
            auto begin = index.begin();
            while(begin != index.end()){
                ret.push_back(exchange_order(*begin));
                begin++;
            }

            return ret;
        }

    }
}