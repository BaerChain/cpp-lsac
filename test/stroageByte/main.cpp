#include <iostream>
#include <libdevcore/TrieCommon.h>
#include <libdevcore/TrieDB.h>
#include <libdevcore/OverlayDB.h>
#include <libdevcore/DBFactory.h>
#include <libbrcdchain/SecureTrieDB.h>
#define fs boost::filesystem

using namespace std;
using namespace dev;
using namespace dev::brc;


const unsigned c_minorProtocolVersion = 2;
const unsigned c_databaseBaseVersion = 9;
const unsigned c_databaseVersionModifier = 0;
const unsigned c_databaseVersion = c_databaseBaseVersion + (c_databaseVersionModifier << 8) + (23 << 9);

int main()
{
    WithExisting _we = WithExisting::Kill;
    string _path = "./db";
    boost::filesystem::path _fspath(_path);\
    h256 _genesisHash = sha3("root");
    if (db::isDiskDatabase() && _we == WithExisting::Kill) {
        clog(VerbosityDebug, "statedb") << "Killing state database (WithExisting::Kill).";
        cwarn << "will remove " << _fspath << "/" << fs::path("state");
        fs::remove_all(_fspath / fs::path("state"));
    }

    _fspath /=
            fs::path(toHex(_genesisHash.ref().cropped(0, 4))) / fs::path(toString(c_databaseVersion));
    if (db::isDiskDatabase()) {
        fs::create_directories(_fspath);
        DEV_IGNORE_EXCEPTIONS(fs::permissions(_fspath, fs::owner_all));
    }

    try{
        std::unique_ptr<db::DatabaseFace> db = db::DBFactory::create(_fspath / fs::path("state"));
            clog(VerbosityTrace, "statedb") << "Opened state DB.";

    RLPStream _rlp(2);
    _rlp << "123456789" << "987654321";
    h256 _hash = sha3(_rlp.out());
    OverlayDB statedb(std::move(db));
    std::cout << "hash = " << _hash << std::endl;
    std::cout << "out = " << _rlp.out() << std::endl;
    SecureTrieDB<h256, OverlayDB> stroageDB(&statedb, EmptyTrie);
    stroageDB.insert(_hash,_rlp.out());
    h256 root = stroageDB.root();
    std::cout << root << std::endl;
    statedb.commit();

    // RLPStream _rlp1(2);
    // _rlp1 << "123" << "321";
    // h256 _hash1 = sha3(_rlp1.out());
    // std::cout << "hash1 = " << _hash1 << std::endl;
    // std::cout << "out1 = " << _rlp1.out() << std::endl;
    // stroageDB.insert(_hash1, _rlp1.out());
    // h256 root1 = stroageDB.root();
    // std::cout << "root1 = " << root1 << std::endl;
    // statedb.commit();


    SecureTrieDB<h256, OverlayDB> stroageDB1(const_cast<OverlayDB*>(&statedb), root1);

    for (auto it = stroageDB1.hashedBegin(); it != stroageDB1.hashedEnd(); ++it)
    {
        // h256 const hashedKey((*it).first);
        // u256 const key = h256(it.key());
        // u256 const value = RLP((*it).second).toInt<u256>();
    }


    // string _res = stroageDB1.at(_hash);
    // std::cout <<  " res1 = "<< RLP(_res).data().toBytes() << std::endl;
    // std::cout << "rlp.out() = " << _rlp.out() << std::endl;

    // bytes _res1 = RLP(_res).data().toBytes();

    // RLP state(_res1);


    // std::cout << "ret = " << state[0].toString() << std::endl;
    // std::cout << "ret = " << state[1].toString() << std::endl;




    }
    catch (boost::exception const &ex) {
        std::cout << boost::diagnostic_information(ex) << '\n';
        if (!db::isDiskDatabase())
            throw;
        else if (fs::space(_fspath / fs::path("state")).available < 1024) {
            std::cout << "Not enough available space found on hard drive. Please free some up and then "
                     "re-run. Bailing.";
        } else {
            std::cout << "Database " << (_fspath / fs::path("state"))
                  << "already open. You appear to have another instance of brcdChain running. "
                     "Bailing." << std::endl;
        }
    }       

    //return OverlayDB(std::move(db));
}