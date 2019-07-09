//
// Created by friday on 25/04/2019.
//

#include <brc/exchangeOrder.hpp>

#include <boost/random.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/FixedHash.h>
namespace dx = dev::brc::ex;

namespace bbfs = boost::filesystem;

dev::Address get_address(size_t i) {
    std::string k = std::to_string(i);
    size_t length = 40;
    std::string ret;
    for (size_t i = 0; i < length - k.length(); i++) {
        ret += "0";
    }
    ret += k;
    return dev::Address(ret);
}


dev::u256 get_random_u256() {
    static int32_t seed = 0;
    boost::mt19937 rng(seed++);
    boost::uniform_int<> ui(1, 100);
    return dev::u256(ui(rng));
}

bool get_random_type() {
    static int32_t seed = 0;
    boost::mt19937 rng(seed++);
    boost::uniform_int<> ui(0, 1);
    return ui(rng);
}


std::vector<dx::order> random_orders(size_t length) {
    std::vector<dx::order> ret;

    for (size_t i = 0; i < length; i++) {
        dx::order o;
        o.sender = get_address(i);
        o.token_type = get_random_type() ? dx::order_token_type::BRC : dx::order_token_type::FUEL;
//        o.buy_type = get_random_type() ? dx::order_buy_type::all_price : dx::order_buy_type::only_price;
        o.buy_type = dx::order_buy_type::only_price;
        o.type = get_random_type() ? dx::order_type::buy : dx::order_type::sell;
        o.price_token[get_random_u256()] = get_random_u256();
        o.time = i;
        o.trxid = dev::h256(i);
        ret.push_back(o);
    }


    return ret;
}


int main(int argc, char *argv[]) {


    bbfs::path cur_dir = bbfs::current_path();
    cur_dir /= bbfs::path("data");
    cur_dir /= bbfs::unique_path();


    try {
        dev::brc::ex::exchange_plugin db(cur_dir);

        auto test = random_orders(200000);
        dev::Timer t;
        for (auto os : test) {
            db.insert_operation({os}, false, true);
        }
        std::cout << " time: " << t.elapsed() << "  size: " << test.size() << std::endl;
        db.commit(1, dev::h256(), dev::h256());

    } catch (const dev::Exception &e) {
        std::cout << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    } catch (const boost::exception &e) {
        std::cout <<  boost::diagnostic_information_what(e) << std::endl;
    }
    catch (...) {
        std::cout << "exit exception.\n";
        exit(1);
    }


    return 0;
}