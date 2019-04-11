#define BOOST_TEST_MODULE test_brc_db
#define BOOST_TEST_DYN_LINK
//#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/random.hpp>
#include <boost/format.hpp>


#include <brc/database.hpp>
#include <libdevcore/Address.h>
#include <brc/exchangeOrder.hpp>
#include <brc/exchangeOrder.hpp>
using namespace chainbase;
using namespace boost::multi_index;
using namespace dev;
using namespace dev::brc::ex;
using namespace std;

struct up_order : public chainbase::object<0, up_order> {

    template<typename Constructor, typename Allocator>
    up_order(Constructor &&c, Allocator &&a):name(a) {
        c(*this);
    }

    id_type id;
    shared_string name;
    dev::Address address;
    uint64_t time;
    uint64_t balance;
    uint8_t type;
    uint64_t price;
};


struct by_id_test;
struct by_name;
typedef multi_index_container<
        up_order,
        indexed_by<
                ordered_unique<tag<by_id_test>,
                        member<up_order, up_order::id_type, &up_order::id>
                >,
                ordered_non_unique<
                        tag<by_name>,
                        composite_key<up_order, member<up_order, shared_string, &up_order::name> >,
                        composite_key_compare<chainbase::strcmp_less>
                >
        >,
        chainbase::allocator<up_order>
>
        up_order_index;

CHAINBASE_SET_INDEX_TYPE(up_order, up_order_index)


namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(test_brc_db)

    BOOST_AUTO_TEST_CASE(db_test1) {
        try {


            //write
            bfs::path cur_dir = bfs::current_path();
            cur_dir /= bfs::unique_path();;


            uint32_t length = 10;
            dev::brc::ex::database db(cur_dir, chainbase::database::read_write, 1024 * 1024 * 1024);
            db.add_index<up_order_index>();
            {
                auto session = db.start_undo_session(true);
                db.create<up_order>([](up_order &order){
                    std::string nn = "xxxxxxxxxxxxxx";
                    order.name.assign(nn.begin(), nn.end());
                });

                session.push();

                db.commit(1);
            }
            {
                auto session = db.start_undo_session(true);
                db.create<up_order>([](up_order &order){
                    std::string nn = "xxxxxxxxxxxxxx2";
                    order.name.assign(nn.begin(), nn.end());
                });

                session.push();

            }
            {
                auto session1 = db.start_undo_session(true);
                for (uint32_t i = 0; i < length; i++) {
                    db.create<up_order>([&](up_order &order) {
                        std::string nn = "test" + std::to_string(i);
                        order.name.assign(nn.begin(), nn.end());

                        order.time = i;
                        order.balance = i;
                        order.type = uint8_t(i & 0xf);
                        order.price = i * 10;
                    });
                }
                session1.push();

            }



//            db.undo();
//            db.flush();


            //read
//            for (uint32_t i = 0; i < length; i++) {
//                std::string nn = "test" + std::to_string(i);
//                const auto itr = db.find<up_order, by_name>(nn);
//                BOOST_CHECK(itr);           //if  not find ,  itr is nullptr,
//                std::cout << itr->name << std::endl;
//                try {
//                    const auto &ret = db.get<up_order, by_name>(nn);
//                } catch (const boost::exception &e) {
//                    //if not find , db.get will throw exception.
//                }
//            }
            const auto &index = db.get_index<up_order_index>().indices().get<by_name>();
            for(auto itr = index.begin(); itr != index.end(); itr++){
                std::cout << itr->name << std::endl;
            }


        } catch (const std::exception &e) {

        } catch (const boost::exception &e) {

        } catch (...) {

        }


    }


    BOOST_AUTO_TEST_CASE(db_test2) {



        std::cout << "Address sizeof : " << sizeof(dev::Address) << std::endl;
        std::cout << "h256 sizeof : " << sizeof(dev::h256) << std::endl;
        std::cout << "u256 sizeof : " << sizeof(dev::u256) << std::endl;
        std::cout << "order_token_type sizeof : " << sizeof(dev::brc::ex::order_token_type) << std::endl;
        std::cout << "price_token sizeof : " << sizeof(dev::brc::ex::order::price_token) << std::endl;
        std::cout << "order sizeof : " << sizeof(dev::brc::ex::order) << std::endl;
        std::cout << "result_order sizeof : " << sizeof(dev::brc::ex::result_order) << std::endl;

        std::cout << "std::array " << sizeof(std::array<uint32_t , 1>) << std::endl;
        std::cout << "std::array " << sizeof(std::array<uint32_t , 5>) << std::endl;
        std::cout << "u256::MAX " << u256(-1) << std::endl;
    }


    BOOST_AUTO_TEST_CASE(db_test3) {
        h256 id("0000000000000000000000000000000000000000000000000000000000000000");
        Address ad("0000000000000000000000000000000000000000");
        order o1 = {id, ad, only_price, BRC, sell, {{1, 1}, {2, 1}, {3, 1}}, 1};
        order o2 = {id, ad, only_price, BRC, buy, {{1, 1}, {2, 1}, {3, 1}}, 2};
        order o3 = {id, ad, only_price, BRC, sell, {{1, 1}, {2, 1}, {3, 1}}, 3};
        order o4 = {id, ad, only_price, BRC, buy, {{1, 1}, {2, 1}, {3, 1}}, 4};

        std::vector<order> os;
        os.push_back(o1);
        os.push_back(o2);
        os.push_back(o3);
        os.push_back(o4);
        try{
            bfs::path cur_dir = bfs::current_path();
            cur_dir += bfs::unique_path();

            dev::brc::ex::database db(cur_dir, chainbase::database::read_write, 1024 * 64);

            for(auto &itr : os){
                if(itr.type == buy){
                    for(auto t : itr.price_token){
                        db.create<order_object>([&](order_object &obj){
//                            obj.trxid = itr.trxid;
//                            obj.sender = itr.sender;
//                            obj.type = itr.type;
//                            obj.token_type = itr.token_type;
//                            obj.price = t.first;
//                            obj.token_amount = t.second;
//                            obj.create_time = itr.time;
                        });
                    }
                }
            }

            const auto &index_itr = db.get_index<order_object_index>().indices().get<by_price_less>();
            auto begin = index_itr.lower_bound(boost::tuple<order_type, order_token_type, u256, uint64_t>(buy, BRC, 2, 0));
            auto end = index_itr.upper_bound(boost::tuple<order_type, order_token_type, u256, uint64_t>(buy, BRC, 2, INT64_MAX));
            std::cout << "---------------------------\n";
            while(begin != end){

                std::cout << "price " << begin->price << std::endl;
                std::cout << "token_amount " << begin->token_amount << std::endl;
                std::cout << "time " << begin->create_time << std::endl;
                std::cout << "---------------------------\n";
                begin++;
            }



        }catch (...){

        }


    }




    BOOST_AUTO_TEST_CASE(db_buy_test4) {
        h256 id("1234500000000000000000000000000000000000000000000000000000001234");
        Address ad("0000000000000000000000000000000000000123");


        auto get_random_price = []() -> h256{
            static int32_t  seed = 0;
            boost::mt19937 rng(seed++);
            boost::uniform_int<> ui(1, 100);
            return h256(ui(rng));
        };

        std::vector<order> os;
        uint32_t    create_size = 1000;
        for(auto i = 0; i < create_size; i++){
            order o;
            o.trxid = id;
            o.sender = ad;
            o.buy_type = only_price;
            o.token_type = FUEL;
            o.type = sell;
            o.price_token = {{get_random_price(), u256(1)}};
            o.time = i;
            os.push_back(o);
        }

        auto check_equal = [](const order & o, const exchange_order &ex) -> bool{
            return o.trxid == ex.trxid &&
                    o.sender == ex.sender &&
                    o.token_type == ex.token_type &&
                    o.time == ex.create_time;

        };




        bfs::path cur_dir = bfs::current_path()  ;
        cur_dir /= bfs::path("data");
        cur_dir /= bfs::unique_path();


        try{
            dev::brc::ex::exchange_plugin db(cur_dir);

            db.insert_operation(os, false, true);



            auto print_data = [&](dev::brc::ex::exchange_plugin &data, bool write_file = false){
                auto exchange_orders = data.get_orders();
//                std::cout << "-------------------------------------------\n";
////                std::cout << "exchange_orders.size:  " << exchange_orders.size() << std::endl;
                std::cout << "--------------------------------------------------------------------------------------------------\n";
                std::cout << boost::format("%|59t|%1% | %|59t|%2% | %3% | %4% | %5% | %6% | %7% | %8%") \
                % "trxid"
                % "sender"
                % "price"
                % "token_amount"
                % "source_amount "
                % "create_time"
                % "type"
                % "token_type"
                << std::endl;
                for(auto &itr : exchange_orders){
                    std::cout  << boost::format("%1% | %2% | %3% | %4% | %5% | %6% | %7% | %8%") \
                    % itr.trxid
                    % itr.sender
                    % itr.price
                    % itr.token_amount
                    % itr.source_amount
                    % itr.create_time
                    % itr.type
                    % itr.token_type
                    << std::flush;
                }
            };

//            print_data(db);

            ///////////////////create sell order
            uint32_t    selle_size = 500;
            for(auto i = 0; i < selle_size ; i++){
                order o;
                o.trxid = id;
                o.sender = Address("0000000000000000400000000000000000001111");
                o.buy_type = only_price;
                o.token_type = BRC;
                o.type = buy;
                for(auto i = 0; i < create_size; i++) {
                    o.price_token = {{get_random_price(), u256(1)}};
                }
                o.time = 0;
                auto ret = db.insert_operation({o}, false, true);

            }

            print_data(db);



        }catch (const boost::exception &e){
            std::cout << "exception ." << boost::diagnostic_information(e) << std::endl;
        }catch(const std::exception &e){
            std::cout << "exception ." << e.what() << std::endl;
        }catch (...){
            std::cout << "unkown exception .\n";
        }

    }


    BOOST_AUTO_TEST_CASE(cout_formmat_test){
        std::cout  << boost::format("%1%.%2%.%3%") % 2018 % 11 % 2 << std::endl;

        std::cout << boost::format("%|40t|%1% | %|20t|%2%") % "trxid"  % "trxid" << std::endl;
        int j=0,i=0;
        while(j++<10)
        {
            printf("这是第%d次输出------",j);
            printf("\r\033[k");
        }
        std::cout << "end" << std::endl;
    }





BOOST_AUTO_TEST_SUITE_END()