//
// Created by fri on 2019/4/11.
//

#include <stdio.h>
#include <iostream>
#include <boost/filesystem.hpp>
#include <brc/exchangeOrder.hpp>
#include <brc/types.hpp>
#include <libdevcore/FixedHash.h>
#include <boost/random.hpp>
#include <boost/format.hpp>
#include <libdevcrypto/base58.h>

namespace bfs = boost::filesystem;
using namespace dev::brc::ex;
using namespace dev;






class test_helper{
public:


    void open_log()
    {
        file_ptr.reset(new std::ofstream(log_name));
    }

    void clean_file_content(){
        file_ptr->close();
        open_log();
    }



    template<typename T>
    std::string sub_h256(const T &h) {
        auto str = h.ref().toString();
        return dev::crypto::to_base58(str.c_str(), str.length()).substr(0, 5) + "...";
    }


    void print_formmat(const std::vector<exchange_order> &orders) {

        auto print_one_line = [&](const exchange_order &e, bool update = false, uint32_t width = 14) {

            *file_ptr << e.trxid.abridged() << " | "
                      << e.sender.abridged() << " | "
                      << std::setw(width) << e.price << " | "
                      << std::setw(width) << e.token_amount << " | "
                      << std::setw(width) << e.source_amount << " | "
                      << std::setw(width) << e.create_time << " | "
                      << std::setw(width) << e.type << " | "
                      << std::setw(width) << e.token_type << " |" << std::endl;

        };

        auto print_header = [&](uint32_t width = 14) {

            *file_ptr   << std::setw(9) << "trxid"
                       << " | "<< std::setw(9) << "sender"
                       << " | "<< std::setw(width) << "price"
                       << " | "<< std::setw(width) << "token_amount"
                       << " | "<< std::setw(width) << "source_amount "
                       << " | "<< std::setw(width) << "create_time"
                       << " | "<< std::setw(width) << "type"
                       << " | "<< std::setw(width) << "token_type" << " |"
                       << std::endl;
        };
        print_header();
        for (const auto &eo : orders) {
            print_one_line(eo, true);
        }
    }


    std::vector<order> get_random_order(uint32_t create_size, order_type type = sell, order_token_type t_type = BRC) {
        h256 id("1234500000000000000000000000000000000000000000000000000000001234");
        Address ad("0000000000000000000000000000000000000123");


        auto get_random_price = [](uint64_t down, uint64_t up) -> h256 {
            static int32_t seed = 0;
            boost::mt19937 rng(seed++);
            boost::uniform_int<> ui(down, up);
            return h256(ui(rng));
        };

        std::vector<order> os;

        for (auto i = 0; i < create_size; i++) {
            order o;
            o.trxid = id;
            o.sender = ad;
            o.buy_type = only_price;
            o.token_type = t_type;
            o.type = type;
            o.price_token = {{get_random_price(10, 20), get_random_price(5, 10)}};
            o.time = i;
            os.push_back(o);
        }
        return os;
    }


    std::string log_name;
    std::shared_ptr<std::ofstream> file_ptr;
};






int main(int argc, char *argv[]) {



    bfs::path cur_dir = bfs::current_path();
    cur_dir /= bfs::path("data");



    test_helper th;
    th.log_name = cur_dir.string() + "/log.log";;
    th.open_log();
    cur_dir /= bfs::unique_path();
    dev::brc::ex::exchange_plugin db(cur_dir);

    std::vector<order> os = th.get_random_order(100, sell, BRC);
    std::vector<order> os1 = th.get_random_order(100, buy, FUEL);
//    std::vector<order> os3 = get_random_order(1000, sell, BRC);
//    std::vector<order> os4 = get_random_order(1000, sell, BRC);


    db.insert_operation(os, false, true);

    th.print_formmat(db.get_order_by_type(sell, BRC, 30));
    db.insert_operation(os1, false, true);
    th.print_formmat(db.get_order_by_type(sell, BRC, 30));


    return 0;
}