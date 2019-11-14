#include <iostream>
#include <libdevcore/db.h>
#include <libdevcore/dbfwd.h>
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <libdevcore/RLP.h>
//#include <boost/test/included/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <libdevcore/Log.h>
#include <libbrcdchain/bplusTree.h>


using  namespace dev;
using  namespace dev::brc;
using namespace dev::db;
//typedef  std::string dbRet;

struct treeTest : public databaseDelegate
{
    treeTest(leveldb::DB *db) :
        m_db(db)
    {

    }
    virtual DataPackage getData(DataKey const& key)
    {
        leveldb::ReadOptions op;
        std::string ret;
        auto status =  m_db->Get(op, key, &ret);
        if(status.ok())
        {
            return DataPackage(ret.c_str(), ret.c_str() + ret.size());
        }
        return DataPackage();

    }

    virtual void setData(DataKey const& key, DataPackage const& value)
    {
        leveldb::WriteOptions wop;
        auto _value = (dev::db::Slice)dev::ref(value);
        m_db->Put(wop, key, leveldb::Slice(_value.data(), _value.size()));
    }

    virtual void deleteKey(DataKey const& key)
    {
        leveldb::WriteOptions wop;
        m_db->Delete(wop, key);
    }

private:
    leveldb::DB *m_db;
};

struct paixu
{
    uint32_t time;
    uint32_t price;

    bool operator<(paixu const& p2) const
    {
        if(time < p2.time)
        {
            return true;
        }else if(time == p2.time)
        {
            if(price < p2.price)
            {
                return true;
            }
        }else{
            return false;
        }
        return false;
    }

    void decode(dev::RLP const& rlp)
    {
        if(rlp.isList())
        {
            time = rlp[0].toInt<uint32_t>();
            price = rlp[1].toInt<uint32_t>();
        }
    }

    void encode(RLPStream& rlp) const
    {
        rlp.appendList(2);
        rlp.append(time);
        rlp.append(price);
    }

    std::string to_string() const
    {
        return "time = " + std::to_string(time) + "   price = " + std::to_string(price);
    }
};

struct xiangqing
{
    std::string name;
    std::string data;

    void decode(RLP const& rlp)
    {
        if(rlp.isList())
        {
            name = rlp[0].toString();
            data = rlp[1].toString();
        }
    }

    void encode(RLPStream& rlp) const
    {
        rlp.appendList(2);
        rlp.append(name);
        rlp.append(data);
    }

    std::string to_string() const
    {
        return "name = " + name + "   data = " + data;
    }
};

BOOST_AUTO_TEST_SUITE(mytest)


    BOOST_AUTO_TEST_CASE(test1)
    {
        leveldb::Options op;
        op.max_open_files = 256;
        op.create_if_missing = true;

        auto db = static_cast<leveldb::DB*>(nullptr);
        auto ret = leveldb::DB::Open(op, "testdb", &db);
        std::cout << "db open status : " <<ret.ok() << std::endl;

        std::shared_ptr<treeTest> testdb(new treeTest(db));

        bplusTree<paixu, xiangqing, 4> _plustree(testdb);

        paixu p1, p2;
        p1.time = 9000;
        p1.price = 300;
        p2.time = 9000;
        p2.price = 200;
        xiangqing x1, x2;
        x1.name = "10000";
        x1.data = "100";
        x2.name = "9000";
        x2.data = "200";

        _plustree.insert(p1, x1);
        _plustree.insert(p2, x2);


        _plustree.debug();
        _plustree.update();

        {

        }
    }


BOOST_AUTO_TEST_SUITE_END()