#define BOOST_TEST_MODULE testTree
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
#include <libbrcdchain/bplusTree.h>
#include <leveldb/db.h>
#include <libdevcore/dbfwd.h>
#include <libdevcore/Log.h>

namespace bbfs = boost::filesystem;


struct virtualDb : public dev::brc::databaseDelegate {

    virtualDb(leveldb::DB *db) : m_db(db) {

    }

    virtual dev::brc::DataPackage getData(const dev::brc::DataKey &nk) {
        if (m_db) {
            leveldb::ReadOptions wo;
            std::string ret;
            auto status = m_db->Get(wo, nk, &ret);
            auto return_r = dev::brc::DataPackage(ret.c_str(), ret.c_str() + ret.size());
            return return_r;
        }

        return dev::brc::DataPackage();
    }

    virtual void setData(const dev::brc::DataKey &nk, const dev::brc::DataPackage &dp) {
        if (m_db) {
            if (!dp.size()) {
                assert(false);
            }
            leveldb::WriteOptions wo;
            auto writeValue = (dev::db::Slice) dev::ref(dp);
            m_db->Put(wo, nk, leveldb::Slice(writeValue.data(), writeValue.size()));
        }
    }

    virtual void deleteKey(const dev::brc::DataKey &nk) {
        if (m_db) {
            leveldb::WriteOptions wo;
            m_db->Delete(wo, nk);
        }
    }

private:
    leveldb::DB *m_db;
};


struct books {
    uint32_t hot = 0;
    uint32_t id = 0;

    void decode(const dev::RLP &rlp) {
        if (rlp.isList()) {
            assert(rlp.itemCount() == 2);
            hot = rlp[0].toInt<uint32_t>();
            id = rlp[1].toInt<uint32_t>();
        }

    }

    void encode(dev::RLPStream &rlp) const{
        rlp.appendList(2);
        rlp.append(hot);
        rlp.append(id);
    }

    bool operator<(const books &b2) const {
        if (hot < b2.hot) {
            return true;
        } else if (hot == b2.hot) {
            if (id < b2.id) {
                return true;
            }
        }
        return false;
    }


    std::string to_string() const {
        return "[ " + std::to_string(hot) + "-" + std::to_string(id) + "]";
    }


};

struct detailbook {
    std::string name;
    std::string from;

    void decode(const dev::RLP &rlp) {
        assert(rlp.itemCount() == 2);
        name = rlp[0].toString();
        from = rlp[1].toString();
    }

    void encode(dev::RLPStream &rlp) const{
        rlp.appendList(2);
        rlp.append(name);
        rlp.append(from);

    }

    std::string to_string() {
        return "[ " + (name) + "-" + (from) + "]";
    }

};

 
struct test_op {
public:
    bool operator > (const test_op &t1) const {
        if(first > t1.first){
            return true;
        }
        else if (first == t1.first){
            return second > t1.second;
        }
        return false;
    }

    bool operator < (const test_op &t1) const {
        if(first < t1.first){
            return true;
        }
        else if (first == t1.first){
            return second < t1.second;
        }
        return false;
    }

    void encode(dev::RLPStream &rlp) const
    {

    }

    void decode(dev::RLP const& _rlp)
    {

    } 

    std::string to_string() const
    {
        return "[" + std::to_string(first) + "," + std::to_string(second) + "]";
    }

    // bool operator () (const unsigned &t1, const unsigned &t2) const {
    //     return t1 < t2;
    // }

    unsigned int first;
    unsigned int second;
};
    

HAS_MEMBER(books);

HAS_MEMBER(detailbook);


BOOST_AUTO_TEST_SUITE(testTree)

    BOOST_AUTO_TEST_CASE(tree_test1) {
        try {


            dev::brc::bplusTree<unsigned, std::string, 4> bp;
            size_t end = 128;
            for (size_t i = 0; i < end; i++) {
                bp.insert(i, std::to_string(i));
            }
            bp.debug();


        } catch (const std::exception &e) {

        } catch (const boost::exception &e) {

        } catch (...) {

        }
    }

    BOOST_AUTO_TEST_CASE(tree_code) {
        try {
            leveldb::Options op;
            op.create_if_missing = true;
            op.max_open_files = 256;


            auto db = static_cast<leveldb::DB *>(nullptr);
            auto ret = leveldb::DB::Open(op, "tdb", &db);
            std::cout << "open db : " << ret.ok() << std::endl;

//            {
//                std::shared_ptr<virtualDb> vdb(new virtualDb(db));
//
//                dev::brc::bplusTree<unsigned, std::string, 4> bp(vdb);
//
//                size_t end = 32;
//                for (size_t i = 0; i < end; i++) {
//                    bp.insert(i, std::to_string(i));
//                }
//
//                bp.debug();
//                bp.update();
//            }


            {
                std::shared_ptr<virtualDb> vdb(new virtualDb(db));
                dev::brc::bplusTree<unsigned, std::string, 4> bp(vdb);

                bp.debug();
                bp.update();
            }

        } catch (const std::exception &e) {

        } catch (const boost::exception &e) {

        } catch (...) {

        }
    }

    BOOST_AUTO_TEST_CASE(tree_code3) {

        try {
            leveldb::Options op;
            op.create_if_missing = true;
            op.max_open_files = 256;


            auto db = static_cast<leveldb::DB *>(nullptr);
            auto ret = leveldb::DB::Open(op, "tdb", &db);
            std::cout << "open db : " << ret.ok() << std::endl;

            {
                std::shared_ptr<virtualDb> vdb(new virtualDb(db));

                dev::brc::bplusTree<books, detailbook, 4> bp(vdb);

                size_t end = 32;
                for (size_t i = 0; i < end; i++) {
                    books b;
                    b.hot = i;
                    b.id = i;
                    detailbook db;
                    db.name = std::to_string(i);
                    db.from = std::to_string(i);
                    bp.insert(b, db);
                }
                bp.debug();
                bp.update();
            }


            {
                std::shared_ptr<virtualDb> vdb(new virtualDb(db));
                dev::brc::bplusTree<books, detailbook, 4> bp(vdb);
                bp.debug();
                bp.update();
            }

        } catch (const std::exception &e) {
            std::cout << "std e " << e.what() << std::endl;
        } catch (const boost::exception &e) {
        } catch (...) {
            std::cout << "std e xxx" << std::endl;
        }



    }


    BOOST_AUTO_TEST_CASE(tree_iter) {
        try {
           

            dev::brc::bplusTree<test_op, std::string, 4, std::less<test_op>> bp;

            bp.insert( {0, 0}, "0#1");
            bp.insert( {1, 0}, "0#2");
            bp.insert( {1, 1}, "0#3");
            bp.insert( {2, 0}, "0#4");
            bp.insert( {2, 3}, "0#5");
            // bp.insert( {3, 0}, "0#6");
      
            bp.debug();
            
            // auto itr = bp.lower_bound(test_op{1, 0});
            // // auto itr = bp.begin();
            // auto itr_end = bp.upper_bound(test_op{999999, 9999});
            // // std::cout << "begin key " << (*itr).first.to_string() << "  value: " << (*itr).second << std::endl;
            // // std::cout << "end key " << (*itr_end).first.to_string() << "  value: " << (*itr_end).second << std::endl;
            // while (itr != itr_end)
            // {
            //     std::cout << "key " << (*itr).first.to_string() << "  value: " << (*itr).second << std::endl;
            //     itr++;
            // }
            


        } catch (const std::exception &e) {
            std::cout << "exception " << e.what() << std::endl;
        } catch (const boost::exception &e) {

        } catch (...) {

        }
    }


BOOST_AUTO_TEST_SUITE_END()