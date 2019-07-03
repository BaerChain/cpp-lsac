#pragma once

#include <boost/filesystem.hpp>


#include <brc/objects.hpp>
//using namespace chainbase;



namespace dev {
    namespace brc {
        namespace ex {


            class database : public chainbase::database {
            public:

                database(const boost::filesystem::path &data_dir, open_flags write = read_only,
                         uint64_t shared_file_size = 0, bool allow_dirty = false);

                ~database();
                std::vector<result_order> find_order(order_type o_type, order_token_type t_type, u256 price_upper);
            };
        }
    }
}