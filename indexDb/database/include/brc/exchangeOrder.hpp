#pragma once


#include <brc/database.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <brc/exception.hpp>
namespace brc{

    using namespace db;


    namespace ex{

        class exchange_plugin{
        public:
            exchange_plugin(const boost::filesystem::path &data_dir);
            std::vector<result_order> insert_operation(const std::vector<order> &orders, bool reset = true, bool throw_exception = false);

        private:
            brc::db::database db;
        };


    }
}