#define BOOST_TEST_MODULE test_brc_db
#define BOOST_TEST_DYN_LINK
//#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <brc/database.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <libdevcore/Address.h>
#include <brc/exchangeOrder.hpp>
#include <boost/tuple/tuple.hpp>
using namespace chainbase;
using namespace boost::multi_index;
using namespace dev;
using namespace brc::db;

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
                >,
                ordered_non_unique<member<up_order, uint64_t, &up_order::time> >
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
            cur_dir += bfs::unique_path();;
            brc::db::database db(cur_dir, chainbase::database::read_write, 1024 * 64);

            db.add_index<up_order_index>();

            for (uint32_t i = 0; i < 0xfff; i++) {
                db.create<up_order>([&](up_order &order) {
                    std::string nn = "test" + std::to_string(i);
                    order.name.assign(nn.begin(), nn.end());

                    order.time = i;
                    order.balance = i;
                    order.type = uint8_t(i & 0xf);
                    order.price = i * 10;
                });
            }
            db.commit(1);
            db.flush();



            //read
            for (uint32_t i = 0; i < 0xfff; i++) {
                std::string nn = "test" + std::to_string(i);
                const auto itr = db.find<up_order, by_name>(nn);
                BOOST_CHECK(itr);           //if  not find ,  itr is nullptr,

                try {
                    const auto &ret = db.get<up_order, by_name>(nn);
                } catch (const boost::exception &e) {
                    //if not find , db.get will throw exception.
                }
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
        std::cout << "order_token_type sizeof : " << sizeof(brc::db::order_token_type) << std::endl;
        std::cout << "price_token sizeof : " << sizeof(brc::db::order::price_token) << std::endl;
        std::cout << "order sizeof : " << sizeof(brc::db::order) << std::endl;
        std::cout << "sell_balance sizeof : " << sizeof(brc::db::result_order::sell_balance) << std::endl;
        std::cout << "result_order sizeof : " << sizeof(brc::db::result_order) << std::endl;
    }


    BOOST_AUTO_TEST_CASE(db_test3) {
        h256 id("0000000000000000000000000000000000000000000000000000000000000000");
        Address ad("0000000000000000000000000000000000000000");
        order o1 = {id, ad, only_price, BRC, sell, {{1, 1}, {2, 1}, {3, 1}}};
        order o2 = {id, ad, only_price, BRC, buy, {{1, 1}, {2, 1}, {3, 1}}};
        order o3 = {id, ad, only_price, BRC, sell, {{1, 1}, {2, 1}, {3, 1}}};
        order o4 = {id, ad, only_price, BRC, buy, {{1, 1}, {2, 1}, {3, 1}}};

        std::vector<order> os;
        os.push_back(o1);
        os.push_back(o2);
        os.push_back(o3);
        os.push_back(o4);
        try{
            bfs::path cur_dir = bfs::current_path();
            cur_dir += bfs::unique_path();;

            brc::db::database db(cur_dir, chainbase::database::read_write, 1024 * 64);

            for(auto &itr : os){
                if(itr.type == buy){
                    for(auto t : itr.price_token){
                        db.create<order_object>([&](order_object &obj){
                            obj.trxid = itr.trxid;
                            obj.sender = itr.sender;
                            obj.buy_method = itr.buy_type;
                            obj.token_type = itr.token_type;
                            obj.price = t.first;
                            obj.token_amount = t.second;
                        });
                    }
                }
            }

            const auto &index_itr = db.get_index<buy_order_object_index>().indices().get<by_price_buy>();
            auto begin = index_itr.lower_bound(boost::tuple<order_token_type, u256, uint64_t>(BRC, 2, UINT64_MAX));
            auto end = index_itr.upper_bound(boost::tuple<order_token_type, u256, uint64_t>(BRC, UINT64_MAX));
            std::cout << "---------------------------\n";
            while(begin != end){

                std::cout << "price " << begin->price << std::endl;
                std::cout << "token_amount " << begin->token_amount << std::endl;
                std::cout << "---------------------------\n";
                begin++;
            }



        }catch (...){

        }


    }










BOOST_AUTO_TEST_SUITE_END()