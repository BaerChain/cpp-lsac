


#include <brc/database.hpp>


namespace brc {
    namespace db {


        database::database(const boost::filesystem::path &data_dir, database::open_flags write,
                           uint64_t shared_file_size, bool allow_dirty) : chainbase::database(data_dir, write,
                                                                                              shared_file_size,
                                                                                              allow_dirty) {
            add_index<order_object_index>();
            add_index<order_result_object_index>();
        }

        database::~database() {}

//        const order_object* database::find_order_by_trxid(const h256 &txid, order_type type) {
//            if(type == buy){
//                return find<order_object, by_trx_id>(txid);
//            }
//            return find<sell_order_object, by_trx_id>(txid);
//        }
//
//        const order_object &database::get_order_by_trxid(const h256 &txid, order_type type) {
//            if(type == buy){
//                return get<order_object, by_trx_id>(txid);
//            }
//            return get<sell_order_object, by_trx_id>(txid);
//        }
//        const order_object *database::find_buy_order_by_trxid(const h256 &txid) {
//            return find<order_object, by_trx_id>(txid);
//        }
//
//        const order_object &database::get_buy_order_by_trxid(const h256 &txid) {
//            return get<order_object, by_trx_id>(txid);
//        }
//
//        const sell_order_object *database::find_sell_order_by_trxid(const h256 &txid) {
//            return find<sell_order_object, by_trx_id>(txid);
//        }
//
//        const sell_order_object &database::get_sell_order_by_trxid(const h256 &txid) {
//            return get<sell_order_object, by_trx_id>(txid);
//        }


    }
}