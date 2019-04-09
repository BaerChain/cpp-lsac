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
            brc::db::database db;
        };


    }
}