#pragma once
#include <chainbase/chainbase.hpp>
#include <boost/filesystem.hpp>

#include <brc/types.hpp>

using namespace chainbase;
namespace brc {
    namespace db {


        class database : public chainbase::database {
        public:
            database(const boost::filesystem::path &data_dir, open_flags write = read_only, uint64_t shared_file_size = 0, bool allow_dirty = false);

            ~database();

//            const order_object* find_buy_order_by_trxid(const h256 &txid);
//            const order_object& get_buy_order_by_trxid(const h256 &txid);
//            const sell_order_object* find_sell_order_by_trxid(const h256 &txid);
//            const sell_order_object& get_sell_order_by_trxid(const h256 &txid);




            //return result_order
            ///create_order(order)

            ///price ,sender(Address),
            ///
            ///find_order

            std::vector<result_order> find_order(order_type o_type, order_token_type t_type, u256 price_upper);


            //delete order
            //delete_order(h256 trxid)



        };
    }
}