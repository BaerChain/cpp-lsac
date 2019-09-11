//
// Creat on 19-9-5.
//

#include <libdevcore/db.h>
#include <libdevcore/CommonJS.h>
#include <libdevcore/DBFactory.h>
#include <libdevcore/TrieHash.h>
#include <boost/filesystem.hpp>
#include <libbrcdchain/State.h>
#include <libdevcore/OverlayDB.h>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <fstream>
#include <libbrcdchain/BlockDetails.h>


using namespace dev;
using namespace dev::brc;
using namespace std;

const std::string key_str = "vote";
const std::string hex_voter = "0x766f746572";
const std::string hex_voted = "0x766f746564";
const std::string hex_vote  = "0x766f7465";
const u256 voter_u256 = u256(hex_voter);
const u256 voted_u256 = u256(hex_voted);
#define ADDRSIZE 51

bytes rlp(u256 value){
    return (RLPStream() << value).out();
}

u256 poputate(const std::string& b){
    return RLP(b).toInt<u256>();
}

Address asAddress(u256 _item)
{
    return right160(h256(_item));
}

u256 fromAddress(Address _a)
{
    return (u160)_a;
}
string fillSize(string const& des){
    string ret = des;
    while (ret.length() < ADDRSIZE -1){
        ret = string("0") +ret;
    }
    return  string("1") + ret;
}
string subAddr(string const& des){
    string ret = des.substr(1, ADDRSIZE-1);
    while (ret.length()){
        if (ret[0] != '0')
            break;
        ret = ret.substr(1,ret.length());
    }
    return  ret;
}

u256 createValue(Address const& addr, u256 flag){
    /// use string jion and to u256
    // address length 52
    std::string v1 = dev::toString(fromAddress(addr));
    std::string ret=  fillSize(v1) + dev::toString(flag);
    return jsToU256(ret);
}

std::pair<Address, u256> parseKey(u256 key){
    string v1 = dev::toString(key);
    assert(v1.length() >= 51);
    string addr = subAddr(v1);
    string flag = v1.substr(51);
    cout << addr << "  "<< flag <<endl;
    return std::make_pair(asAddress(jsToU256(addr)), jsToU256(flag));
}

void insert10message(SecureTrieDB<h256, OverlayDB>& stateDb){
    std::map<Address, u256> ads = {
            {Address("4e4f68b656544955ff17ba3d92b737a278e2d294"),1},
            {Address("bd3881a04a915d31f2b13580d71f97f1362a9ba3"),2},
            {Address("77ca2d3c5c0f07f68bcf7e328a36f6f49d09d131"),3},
            {Address("81d9d4fa405e4f6f171b6d5fd2148768fbfa58b8"),4},
            {Address("5482857914792fe66858dfc24e1b485ed78fc1da"),5},
            {Address("7735a83b733800443e3e0f316249e820084026a6"),6},
            {Address("9024eda9e6a80c1caf8fb7c8cb8817af63623e6b"),7},
            {Address("cc29ca6673ad78916f332f70c00c6e3054f19989"),8},
            {Address("db700a26f9e7841e7ea7b44b026b8c128ab37017"),9},
            {Address("62b774c4bcc85a9a7e23168796066bb9c571e079"),10}
    };
    h256  root_10= dev::sha3("insert10message");
    assert(!stateDb.isEmpty());
    for(auto const& a :ads){
        stateDb.insert(fromAddress(a.first), rlp(createValue(a.first, a.second)));
        cout << dev::toString(createValue(a.first, a.second))<<endl;
    }
    cout<<endl;

    for(auto const& a : ads){
        std::cout << poputate(stateDb[fromAddress(a.first)]) << endl;
    }

}

void initSecureTrieDB(std::string root, OverlayDB& db){
    assert(2);
    //OverlayDB test_db;
    // static OverlayDB openDB(boost::filesystem::path const& _path, h256 const& _genesisHash, WithExisting _we = WithExisting::Trust);
    db = State::openDB(boost::filesystem::path("./testStoreDb"), dev::sha3(root), WithExisting::Trust);
    SecureTrieDB<h256, OverlayDB> m_state(&db);
    m_state.init();
    /// The map with is overlaid onto whatever storage is implied by the m_storageRoot in the trie.
    //std::unordered_map<u256, u256> m_storage;
    if (!m_state.contains(u256(hex_vote))){
        std::cout << "insert init data...";
        u256  first_key = u256(hex_vote);
        u256 first_value = 4353;
        m_state.insert(first_key, rlp(first_value));
    }

    auto ret = m_state[u256(hex_vote)];
    std::cout << "\n root ret:"<< poputate(ret) << endl;

    //insert10message(m_state);
    m_state.db()->commit();
    //m_state.close();

}

std::map<Address , u256> createAddress(int num){
    std::fstream fs;
    fs.open("test_address.txt", ios::in );
    if (!fs){
        fs.open("test_address.txt", ios::out);
    }
    std::map<Address , u256> ads;
    for (int i =0; i < 1000; i++){
        auto key_pair = KeyPair::create();
        auto sec = key_pair.secret();
        ads[key_pair.address()] = i+1;
        fs << dev::toString(key_pair.address()) << "  "<< dev::toString(i+1);
    }
    return  ads;
}


void test1000message(string const& root){
    auto  db = State::openDB(boost::filesystem::path("./testStoreDb"), dev::sha3(root), WithExisting::Trust);
    SecureTrieDB<h256, OverlayDB> m_state(&db);
    //std::cout << poputate(stateDb[fromAddress(a.first)]) << endl;
    std::map<Address , u256> ads = createAddress(1000);
    cout << "start insert..."<<endl;
    int64_t time1 = utcTimeMilliSec();
    for(auto const& a :ads){
        m_state.insert(fromAddress(a.first), rlp(createValue(a.first, a.second)));
    }
    cout << "insert use time:"<< utcTimeMilliSec() - time1 <<endl;
}

void test_fill(){
    //测试 数据压缩为 u256
    //测试解压
    u256  key=  createValue(Address("00000000000000000e3e0f316249e820080426a6"), 0);
    cout << " voter:" << voter_u256 <<endl;
    auto ret= parseKey(key);
    cout << " key :" <<dev::toString(key)<<endl;
    cout << dev::toString(ret.first)<< "  "<< ret.second;

/// 不同地址转换为 u256 后长度不一致
    Address test1 = Address("fffffffffffffffffffffffffffffffffffffffe");
    Address test2 = Address("7735a83b733800443e3e0f316249e820080426a6");
    Address test3 = Address("00000000000000000e3e0f316249e820080426a6");
}

int main(int arg, char * argv[]){
//    OverlayDB db;
//    initSecureTrieDB("root", db);
//    test1000message("root");
    cout << sizeof(BlockHeader) << endl;
    cout << sizeof(dev::brc::BlockDetails) <<endl;
    cout << sizeof(dev::brc::BlockLogBlooms)<<endl;
    return  0;
}



