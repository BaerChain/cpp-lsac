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

    virtualDb(leveldb::DB *db):m_db(db){

    }

    virtual dev::brc::DataPackage getData(const dev::brc::DataKey &nk) {
        if(m_db){
            leveldb::ReadOptions wo;
            std::string ret;
            auto status = m_db->Get(wo, nk, &ret);
//            if (status.IsNotFound()){
//                std::cout << "cant find key " << nk << std::endl;
//                assert(false);
//                exit(1);
//            }
            auto return_r = dev::brc::DataPackage(ret.c_str(), ret.c_str() + ret.size());
            return return_r;
        }

        return dev::brc::DataPackage();
    }

    virtual void setData(const dev::brc::DataKey &nk, const dev::brc::DataPackage &dp) {
        if(m_db){
//            std::cout << "write " << nk  << " value " << dp.data() << std::endl;
//            cwarn << "write " << nk  << " value " << dp;
            if(!dp.size()){
                assert(false);
            }
            leveldb::WriteOptions wo;
            auto writeValue = (dev::db::Slice) dev::ref(dp);
            m_db->Put(wo, nk, leveldb::Slice(writeValue.data(), writeValue.size()));
        }
    }

    virtual void deleteKey(const dev::brc::DataKey &nk) {
        if(m_db){
            leveldb::WriteOptions wo;
            m_db->Delete(wo, nk);
        }
    }

private:
    leveldb::DB *m_db;
};

BOOST_AUTO_TEST_SUITE(testTree)

    BOOST_AUTO_TEST_CASE(tree_test1) {
        try {


            dev::brc::bplusTree<size_t, std::string, 4> bp;
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


            auto db = static_cast<leveldb::DB*>(nullptr);
            auto ret = leveldb::DB::Open(op, "tdb", &db);
            std::cout << "open db : " << ret.ok() << std::endl;

            {
                std::shared_ptr<virtualDb>  vdb(new virtualDb(db));

                dev::brc::bplusTree<unsigned, std::string, 4> bp(vdb);

                size_t end = 32;
                for (size_t i = 0; i < end; i++) {
                    bp.insert(i, std::to_string(i));
                }

                bp.debug();
                bp.update();
            }


            {
                std::shared_ptr<virtualDb>  vdb(new virtualDb(db));
                dev::brc::bplusTree<unsigned, std::string, 4> bp(vdb);
                auto rootKey = vdb->getData("rootKey");
                if(rootKey.size()){
                    bp.setRootKey(rootKey);
                }
                bp.debug();
                bp.update();
            }





        } catch (const std::exception &e) {

        } catch (const boost::exception &e) {

        } catch (...) {

        }
    }


BOOST_AUTO_TEST_SUITE_END()