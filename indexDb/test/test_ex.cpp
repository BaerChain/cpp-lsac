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

namespace bbfs = boost::filesystem;
using namespace dev::brc::ex;
using namespace dev;



#define SUB_STR_LENGTH      5
template<typename T>
std::string enum_to_string(T type) {
    return "null";
}


template<>
std::string enum_to_string<order_type>(order_type type) {
    return type == order_type::buy ? "buy" : "sell";
}

template<>
std::string enum_to_string<order_token_type>(order_token_type type) {
    return type == order_token_type::BRC ? "BRC" : "FUEL";
}

template<>
std::string enum_to_string<order_buy_type>(order_buy_type type) {
    return type == order_buy_type::all_price ? "all" : "only";
}


class test_helper {
public:


    void open_log() {
        file_ptr.reset(new std::ofstream(log_name));
    }

    void clean_file_content() {
        file_ptr->close();
        open_log();
    }


    template<typename T>
    std::string sub_h256(const T &h) {
        auto str = h.ref().toString();
        return dev::crypto::to_base58(str.c_str(), str.length()).substr(0, 5) + "...";
    }


    void print_formmat(const std::vector<exchange_order> &orders) {
        system("clear");
        auto print_one_line = [&](const exchange_order &e, bool update = false, uint32_t width = 14) {

            std::cout << toHex(e.trxid).substr(0, SUB_STR_LENGTH) << " | "
                      << toHex(e.sender).substr(0, SUB_STR_LENGTH) << " | "
                      << std::setw(width) << e.price << " | "
                      << std::setw(width) << e.token_amount << " | "
                      << std::setw(width) << e.source_amount << " | "
                      << std::setw(width) << e.create_time << " | "
                      << std::setw(width) << enum_to_string(e.type) << " | "
                      << std::setw(width) << enum_to_string(e.token_type) << " |" << std::endl;

        };

        auto print_header = [&](uint32_t width = 14) {

            std::cout << std::setw(5) << "trxid"
                      << " | " << std::setw(5) << "sender"
                      << " | " << std::setw(width) << "price"
                      << " | " << std::setw(width) << "token_amount"
                      << " | " << std::setw(width) << "source_amount "
                      << " | " << std::setw(width) << "create_time"
                      << " | " << std::setw(width) << "type"
                      << " | " << std::setw(width) << "token_type" << " |"
                      << std::endl;
        };
        print_header();
        for (const auto &eo : orders) {
            print_one_line(eo, true);
        }
    }

    void print_sys_result(const std::vector<result_order> &ret) {
        std::cout << "\n\n";
        auto print_one_line = [&](const result_order &e, bool update = false, uint32_t width = 14) {

            std::cout << toHex(e.sender).substr(0, SUB_STR_LENGTH) << " | "
                      << toHex(e.acceptor).substr(0, SUB_STR_LENGTH) << " | "
                      << std::setw(width) << e.price << " |"
                      << std::setw(width) << enum_to_string(e.type) << " | "
                      << std::setw(width) << enum_to_string(e.token_type) << " | "
                      << std::setw(width) << enum_to_string(e.buy_type) << " | "
                      << std::setw(width) << toHex(e.send_trxid).substr(0, SUB_STR_LENGTH) << " | "
                      << std::setw(width) << toHex(e.to_trxid).substr(0, SUB_STR_LENGTH) << " | "
                      << std::setw(width) << e.amount << " |"

                      << std::endl;

        };



        auto print_header = [&](uint32_t width = 14) {

            std::cout << std::setw(5) << "sender"
                      << " | " << std::setw(5) << "acceptor"
                      << " | " << std::setw(width) << "price"
                      << " | " << std::setw(width) << "type"
                      << " | " << std::setw(width) << "token_type"
                      << " | " << std::setw(width) << "buy_type "
                      << " | " << std::setw(width) << "send_trxid"
                      << " | " << std::setw(width) << "to_trxid"
                      << " | " << std::setw(width) << "amount"
                      << " |"
                      << std::endl;
        };


        print_header();
        for (const auto itr : ret) {
            print_one_line(itr);
        }


    }


    std::vector<order> get_random_order(uint32_t create_size, order_type type = order_type::sell, order_token_type t_type = order_token_type::BRC, order_buy_type buy_type = order_buy_type::only_price) {


        auto get_random_price = [](uint64_t down, uint64_t up) -> h256 {
            static int32_t seed = 0;
            boost::mt19937 rng(seed++);
            boost::uniform_int<> ui(down, up);
            return h256(ui(rng));
        };

        auto get_random_str = [](bool hash) {
            static int32_t seed = 0;
            boost::mt19937 rng(seed++);
            boost::uniform_int<uint64_t> ui(10000000UL, 99999999UL);
            if (hash) {
                return std::to_string(ui(rng)) + "00000000000000000000000000000000000000000000000000001234";
            }
            return std::to_string(ui(rng)) + "00000000000000000000000000000000";
        };

        h256 id("1234500000000000000000000000000000000000000000000000000000001234");
        Address ad("0000000000000000000000000000000000000123");

        std::vector<order> os;

        for (auto i = 0; i < create_size; i++) {
            order o;
            o.trxid = h256(get_random_str(true));
            o.sender = Address(get_random_str(false));
            o.buy_type = buy_type;
            o.token_type = t_type;
            o.type = type;
            o.price_token = {{get_random_price(1000, 2000), get_random_price(500, 1000)}};
            o.time = i;
            os.push_back(o);
        }
        return os;
    }


    std::string log_name;
    std::shared_ptr<std::ofstream> file_ptr;
};


int main(int argc, char *argv[]) {

    system("export TERM=linux");


    bbfs::path cur_dir = bbfs::current_path();
    cur_dir /= bbfs::path("data");


    test_helper th;
    th.log_name = cur_dir.string() + "/log.log";;
    th.open_log();
    cur_dir /= bbfs::unique_path();
    dev::brc::ex::exchange_plugin db(cur_dir);

    std::vector<order> os = th.get_random_order(200, order_type::sell, order_token_type::BRC, order_buy_type::only_price);
//    std::vector<order> os1 = th.get_random_order(100, buy, FUEL);
//    std::vector<order> os3 = get_random_order(1000, sell, BRC);
//    std::vector<order> os4 = get_random_order(1000, sell, BRC);


    db.insert_operation(os, false, true);

    th.print_formmat(db.get_order_by_type(order_type::sell, order_token_type::BRC, 30));

    int count = 100;
    while (count--) {
        sleep(2);
        auto ret = db.insert_operation(th.get_random_order(1, order_type::buy, order_token_type::FUEL, order_buy_type::all_price), false, true);
        auto goyt = db.get_order_by_type(order_type::sell, order_token_type::BRC, 30);
        th.print_formmat(goyt);
        th.print_sys_result(db.get_result_orders_by_news(5));
    }


    std::cout << "end\n";

    return 0;
}