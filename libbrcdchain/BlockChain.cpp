#include "BlockChain.h"

#include "Block.h"
#include "GenesisInfo.h"
#include "ImportPerformanceLogger.h"
#include "State.h"
#include "SecureTrieDB.h"
#include "ThreadPackTransactions.h"
#include <libdevcore/Assertions.h>
#include <libdevcore/Common.h>
#include <libdevcore/DBFactory.h>
#include <libdevcore/FileSystem.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieHash.h>
#include <libbrccore/BlockHeader.h>
#include <libbrccore/Exceptions.h>
#include <libdevcore/dbfwd.h>
#include <boost/exception/errinfo_nested_exception.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace dev::db;

namespace fs = boost::filesystem;

#define BRC_TIMED_IMPORTS 1

namespace {
    std::string const c_chainStart{"chainStart"};
    db::Slice const c_sliceChainStart{c_chainStart};
}

std::ostream &dev::brc::operator<<(std::ostream &_out, BlockChain const &_bc) {
    string cmp = toBigEndianString(_bc.currentHash());
    _bc.m_blocksDB->forEach([&_out, &cmp](db::Slice const &_key, db::Slice const &_value) {
        if (string(_key.data(), _key.size()) != "best") {
            const string key(_key.data(), _key.size());
            try {
                BlockHeader d(bytesConstRef{_value});
                _out << toHex(key) << ":   " << d.number() << " @ " << d.parentHash()
                     << (cmp == key ? "  BEST" : "") << std::endl;
            }
            catch (...) {
                cwarn << "Invalid DB entry:" << toHex(key) << " -> "
                      << toHex(bytesConstRef(_value));
            }
        }
        return true;
    });
    return _out;
}

db::Slice dev::brc::toSlice(h256 const &_h, unsigned _sub) {
#if ALL_COMPILERS_ARE_CPP11_COMPLIANT
    static thread_local FixedHash<33> h = _h;
    h[32] = (uint8_t)_sub;
    return (db::Slice)h.ref();
#else
    static boost::thread_specific_ptr<FixedHash<33>> t_h;
    if (!t_h.get())
        t_h.reset(new FixedHash<33>);
    *t_h = FixedHash<33>(_h);
    (*t_h)[32] = (uint8_t) _sub;
    return (db::Slice) t_h->ref();
#endif //ALL_COMPILERS_ARE_CPP11_COMPLIANT
}

db::Slice dev::brc::toSlice(uint64_t _n, unsigned _sub) {
#if ALL_COMPILERS_ARE_CPP11_COMPLIANT
    static thread_local FixedHash<33> h;
    toBigEndian(_n, bytesRef(h.data() + 24, 8));
    h[32] = (uint8_t)_sub;
    return (db::Slice)h.ref();
#else
    static boost::thread_specific_ptr<FixedHash<33>> t_h;
    if (!t_h.get())
        t_h.reset(new FixedHash<33>);
    bytesRef ref(t_h->data() + 24, 8);
    toBigEndian(_n, ref);
    (*t_h)[32] = (uint8_t) _sub;
    return (db::Slice) t_h->ref();
#endif
}

namespace {

    class LastBlockHashes : public LastBlockHashesFace {
    public:
        explicit LastBlockHashes(BlockChain const &_bc) : m_bc(_bc) {}

        h256s precedingHashes(h256 const &_mostRecentHash) const override {
            Guard l(m_lastHashesMutex);
            if (m_lastHashes.empty() || m_lastHashes.front() != _mostRecentHash) {
                m_lastHashes.resize(256);
                m_lastHashes[0] = _mostRecentHash;
                for (unsigned i = 0; i < 255; ++i)
                    m_lastHashes[i + 1] = m_lastHashes[i] ? m_bc.info(m_lastHashes[i]).parentHash() : h256();
            }
            return m_lastHashes;
        }

        void clear() override {
            Guard l(m_lastHashesMutex);
            m_lastHashes.clear();
        }

    private:
        BlockChain const &m_bc;

        mutable Mutex m_lastHashesMutex;
        mutable h256s m_lastHashes;
    };

    void addBlockInfo(Exception &io_ex, BlockHeader const &_header, bytes &&_blockData) {
        io_ex << errinfo_now(time(0));
        io_ex << errinfo_block(std::move(_blockData));
        // only populate extraData if we actually managed to extract it. otherwise,
        // we might be clobbering the existing one.
        if (!_header.extraData().empty())
            io_ex << errinfo_extraData(_header.extraData());
    }

}


/// Duration between flushes.
static const chrono::system_clock::duration c_collectionDuration = chrono::seconds(60);

/// Length of death row (total time in cache is multiple of this and collection duration).
static const unsigned c_collectionQueueSize = 20;

/// Max size, above which we start forcing cache reduction.
static const unsigned c_maxCacheSize = 1024 * 1024 * 64;

/// Min size, below which we don't bother flushing it.
static const unsigned c_minCacheSize = 1024 * 1024 * 32;


BlockChain::BlockChain(ChainParams const &_p, fs::path const &_dbPath, WithExisting _we, ProgressCallback const &_pc) :
        m_lastBlockHashes(new LastBlockHashes(*this)),
        m_dbPath(_dbPath) {
    init(_p);
    open(_dbPath, _we, _pc);
}

BlockChain::~BlockChain() {
    close();
}

void BlockChain::clean_cached_blocks(const dev::OverlayDB &_stateDB, dev::brc::ex::exchange_plugin &_stateExDB) {
    cwarn << "try to close block chain, will remove cached blocks";
    OverlayDB db(_stateDB);
    size_t position = 0;
    if(m_cached_blocks.size() > 1){
        for(auto i = 0; i < m_cached_blocks.size(); i++){
            if(m_cached_blocks[i].size() > m_cached_blocks[position].size() ){
                position = i;
            }
        }
    }

    cwarn << "max_chain position " << position;
    if(m_cached_blocks.size() > 0){
        cwarn << "rollback number: " << info().number();
        remove_blocks_from_database(m_cached_blocks[position], _stateDB, _stateExDB);
        auto last_config_hash = numberHash(info().number() - m_params.config_blocks);
        cwarn << "close " << info().number() - m_params.config_blocks << " hash " << last_config_hash;
        m_extrasDB->insert(db::Slice("best"), db::Slice((char const *) &last_config_hash, 32));

        //remove children
        std::unique_ptr<db::WriteBatchFace> extrasWriteBatch = m_extrasDB->createWriteBatch();
        auto last_block_detail = m_details[last_config_hash];
        last_block_detail.children.clear();
        extrasWriteBatch->insert(toSlice(last_config_hash, ExtraDetails), (db::Slice) dev::ref(last_block_detail.rlp()));
        m_extrasDB->commit(std::move(extrasWriteBatch));

        //rollback exdb
        _stateExDB.remove_all_session();
    }
    else{
        cwarn << "waring m_cached_blocks cant remove ...， close database exception.";
    }
}

BlockHeader const &BlockChain::genesis() const {
    UpgradableGuard l(x_genesis);
    if (!m_genesis) {
        auto gb = m_params.genesisBlock();
        UpgradeGuard ul(l);
        m_genesis = BlockHeader(gb);
        m_genesisHeaderBytes = BlockHeader::extractHeader(&gb).data().toBytes();
        m_genesisHash = m_genesis.hash();
    }
    return m_genesis;
}

void BlockChain::init(ChainParams const &_p) {
    // initialise deathrow.
    m_cacheUsage.resize(c_collectionQueueSize);
    m_lastCollection = chrono::system_clock::now();

    // Initialise with the genesis as the last block on the longest chain.
    m_params = _p;
    m_sealEngine.reset(m_params.createSealEngine());
    m_genesis.clear();
    genesis();
}

unsigned BlockChain::open(fs::path const &_path, WithExisting _we) {
    fs::path path = _path.empty() ? db::databasePath() : _path;
    fs::path chainPath = path / fs::path(toHex(m_genesisHash.ref().cropped(0, 4)));
    fs::path extrasPath = chainPath / fs::path(toString(c_databaseVersion));
    unsigned lastMinor = c_minorProtocolVersion;

    if (db::isDiskDatabase()) {
        fs::create_directories(extrasPath);
        DEV_IGNORE_EXCEPTIONS(fs::permissions(extrasPath, fs::owner_all));

        bytes status = contents(extrasPath / fs::path("minor"));
        if (!status.empty())
            DEV_IGNORE_EXCEPTIONS(lastMinor = (unsigned) RLP(status));
        if (c_minorProtocolVersion != lastMinor) {
            cnote << "Killing extras database (DB minor version:" << lastMinor << " != our miner version: "
                  << c_minorProtocolVersion << ").";
            DEV_IGNORE_EXCEPTIONS(fs::remove_all(extrasPath / fs::path("details.old")));
            fs::rename(extrasPath / fs::path("extras"), extrasPath / fs::path("extras.old"));
            fs::remove_all(extrasPath / fs::path("state"));
            writeFile(extrasPath / fs::path("minor"), rlp(c_minorProtocolVersion));
            lastMinor = (unsigned) RLP(status);
        }

        if (_we == WithExisting::Kill) {
            cnote << "Killing blockchain & extras database (WithExisting::Kill).";
            fs::remove_all(chainPath / fs::path("blocks"));
            fs::remove_all(extrasPath / fs::path("extras"));
        }
    }

    try {
        m_blocksDB = db::DBFactory::create(chainPath / fs::path("blocks"));
        m_extrasDB = db::DBFactory::create(extrasPath / fs::path("extras"));
    }
    catch (db::DatabaseError const &ex) {
        // Check the exact reason of errror, in case of IOError we can display user-friendly message
        if (*boost::get_error_info<db::errinfo_dbStatusCode>(ex) != db::DatabaseStatus::IOError)
            throw;

        if (db::isDiskDatabase()) {
            if (fs::space(chainPath / fs::path("blocks")).available < 1024) {
                cwarn
                    << "Not enough available space found on hard drive. Please free some up and then re-run. Bailing.";
                BOOST_THROW_EXCEPTION(NotEnoughAvailableSpace());
            } else {
                cwarn <<
                      "Database " <<
                      (chainPath / fs::path("blocks")) <<
                      "or " <<
                      (extrasPath / fs::path("extras")) <<
                      "already open. You appear to have another instance of brcdChain running. Bailing.";
                BOOST_THROW_EXCEPTION(DatabaseAlreadyOpen());
            }
        } else {
            cwarn << "Unknown database error occurred during in-memory database creation";
            throw;
        }
    }

    if (_we != WithExisting::Verify && !details(m_genesisHash)) {
        BlockHeader gb(m_params.genesisBlock());
        // Insert details of genesis block.
        m_details[m_genesisHash] = BlockDetails(0, gb.difficulty(), h256(), {});
        auto r = m_details[m_genesisHash].rlp();
        m_extrasDB->insert(toSlice(m_genesisHash, ExtraDetails), (db::Slice) dev::ref(r));
        assert(isKnown(gb.hash()));
    }

#if BRC_PARANOIA
    checkConsistency();
#endif

    // TODO: Implement ability to rebuild details map from DB.
    auto const l = m_extrasDB->lookup(db::Slice("best"));
    m_lastBlockHash = l.empty() ? m_genesisHash : h256(l, h256::FromBinary);

    m_lastBlockNumber = number(m_lastBlockHash);

    ctrace << "Opened blockchain DB. Latest: " << currentHash()
           << (lastMinor == c_minorProtocolVersion ? "(rebuild not needed)" : "*** REBUILD NEEDED ***");
    return lastMinor;
}

void BlockChain::open(fs::path const &_path, WithExisting _we, ProgressCallback const &_pc) {
    if (open(_path, _we) != c_minorProtocolVersion || _we == WithExisting::Verify)
        rebuild(_path, _pc);
}

void BlockChain::reopen(ChainParams const &_p, WithExisting _we, ProgressCallback const &_pc) {
    close();
    init(_p);
    open(m_dbPath, _we, _pc);
}

void BlockChain::close() {
    ctrace << "Closing blockchain DB";
    // Not thread safe...
    m_extrasDB.reset();
    m_blocksDB.reset();
    DEV_WRITE_GUARDED(x_lastBlockHash) {
        m_lastBlockHash = m_genesisHash;
        m_lastBlockNumber = 0;
    }
    m_details.clear();
    m_blocks.clear();
    m_logBlooms.clear();
    m_receipts.clear();
    m_transactionAddresses.clear();
    m_blockHashes.clear();
    m_blocksBlooms.clear();
    m_cacheUsage.clear();
    m_inUse.clear();
    m_lastBlockHashes->clear();

}

void BlockChain::rebuild(fs::path const &_path, std::function<void(unsigned, unsigned)> const &_progress) {
    if (!db::isDiskDatabase()) {
        cwarn << "In-memory database detected, skipping rebuild (since there's no existing database to rebuild)";
        return;
    }

    fs::path path = _path.empty() ? db::databasePath() : _path;
    fs::path chainPath = path / fs::path(toHex(m_genesisHash.ref().cropped(0, 4)));
    fs::path extrasPath = chainPath / fs::path(toString(c_databaseVersion));

    unsigned originalNumber = m_lastBlockNumber;

    ///////////////////////////////
    // TODO
    // - KILL ALL STATE/CHAIN
    // - REINSERT ALL BLOCKS
    ///////////////////////////////

    // Keep extras DB around, but under a temp name
    m_extrasDB.reset();
    fs::rename(extrasPath / fs::path("extras"), extrasPath / fs::path("extras.old"));
    std::unique_ptr<db::DatabaseFace> oldExtrasDB(db::DBFactory::create(extrasPath / fs::path("extras.old")));
    m_extrasDB = db::DBFactory::create(extrasPath / fs::path("extras"));

    // Open a fresh state DB

    Block s = genesisBlock(State::openDB(path.string(), m_genesisHash, WithExisting::Kill),
                           State::openExdb(fs::path(path.string() + std::string("/exdb")), WithExisting::Kill));

    // Clear all memos ready for replay.
    m_details.clear();
    m_logBlooms.clear();
    m_receipts.clear();
    m_transactionAddresses.clear();
    m_blockHashes.clear();
    m_blocksBlooms.clear();
    m_lastBlockHashes->clear();
    m_lastBlockHash = genesisHash();
    m_lastBlockNumber = 0;

    m_details[m_lastBlockHash].totalDifficulty = s.info().difficulty();
    m_details[m_lastBlockHash].number = s.info().number();
    m_extrasDB->insert(toSlice(m_lastBlockHash, ExtraDetails),
                       (db::Slice) dev::ref(m_details[m_lastBlockHash].rlp()));

    h256 lastHash = m_lastBlockHash;
    Timer t;
    for (unsigned d = 1; d <= originalNumber; ++d) {
        if (!(d % 1000)) {
            cerr << "\n1000 blocks in " << t.elapsed() << "s = " << (1000.0 / t.elapsed()) << "b/s" << endl;
            t.restart();
        }
        try {
            bytes b = block(queryExtras<BlockHash, uint64_t, ExtraBlockHash>(
                    d, m_blockHashes, x_blockHashes, NullBlockHash, oldExtrasDB.get())
                                    .value);

            BlockHeader bi(&b);

            if (bi.parentHash() != lastHash) {
                cwarn << "DISJOINT CHAIN DETECTED; " << bi.hash() << "#" << d << " -> parent is" << bi.parentHash()
                      << "; expected" << lastHash << "#" << (d - 1);
                return;
            }
            lastHash = bi.hash();
            cerror << " import begin blockchain import";
            import(b, s.db(), s.exdb(), 0);
        }
        catch (...) {
            // Failed to import - stop here.
            break;
        }

        if (_progress)
            _progress(d, originalNumber);
    }

    fs::remove_all(path / fs::path("extras.old"));
}

string BlockChain::dumpDatabase() const {
    ostringstream oss;
    oss << m_lastBlockHash << '\n';

    // We need to first insert the db data into an ordered map so that the string returned from this function
    // always has data in the same order, regardless of the underlying database implementation
    std::map<std::string, std::string> dbData;
    m_extrasDB->forEach([&dbData](db::Slice key, db::Slice value) {
        dbData[key.toString()] = value.toString();
        return true;
    });

    for (auto const &it : dbData)
        oss << toHex(it.first) << "/" << toHex(it.second) << '\n';

    return oss.str();
}

tuple<ImportRoute, bool, unsigned>
BlockChain::sync(BlockQueue &_bq, OverlayDB const &_stateDB, ex::exchange_plugin &_stateExDB, unsigned _max) {
//  _bq.tick(*this);
    VerifiedBlocks blocks;
    _bq.drain(blocks, _max);

    h256s fresh;
    h256s dead;
    h256s badBlocks;
    Transactions goodTransactions;
    unsigned count = 0;
    for (VerifiedBlock const &block: blocks) {
        do {
            try {
                // Nonce & uncle nonces already verified in verification thread at this point.
                ImportRoute r;
                //DEV_TIMED_ABOVE("Block import " + toString(block.verified.info.number()), 500)
                r = import(block.verified, _stateDB, _stateExDB,
                           (ImportRequirements::Everything &
                            ~ImportRequirements::ValidSeal &
                            ~ImportRequirements::CheckUncles) != 0);
                fresh += r.liveBlocks;
                dead += r.deadBlocks;
                goodTransactions.reserve(goodTransactions.size() + r.goodTranactions.size());
                std::move(std::begin(r.goodTranactions), std::end(r.goodTranactions),
                          std::back_inserter(goodTransactions));
                ++count;
            }
            catch (dev::brc::AlreadyHaveBlock const &) {
                cwarn << "ODD: Import queue contains already imported block";
                continue;
            }
            catch (dev::brc::UnknownParent const &) {
                cwarn
                    << "ODD: Import queue contains block with unknown parent.";// << LogTag::Error << boost::current_exception_diagnostic_information();
                // NOTE: don't reimport since the queue should guarantee everything in the right order.
                // Can't continue - chain bad.
                badBlocks.push_back(block.verified.info.hash());
            }
            catch (dev::brc::FutureTime const &) {
                cwarn << "ODD: Import queue contains a block with future time.";
                this_thread::sleep_for(chrono::milliseconds(200));
                continue;
            }
            catch (dev::brc::TransientError const &) {
                this_thread::sleep_for(chrono::milliseconds(50));
                continue;
            }
            catch (Exception &ex) {
//              cnote << "Exception while importing block. Someone (Jeff? That you?) seems to be giving us dodgy blocks!";// << LogTag::Error << diagnostic_information(ex);
                if (m_onBad)
                    m_onBad(ex);
                // NOTE: don't reimport since the queue should guarantee everything in the right order.
                // Can't continue - chain  bad.
                badBlocks.push_back(block.verified.info.hash());
            }
        } while (false);
    }
    return make_tuple(ImportRoute{dead, fresh, goodTransactions}, _bq.doneDrain(badBlocks), count);
}

pair<ImportResult, ImportRoute>
BlockChain::attemptImport(bytes const &_block, OverlayDB const &_stateDB, ex::exchange_plugin &_stateExDB,
                          bool _mustBeNew) noexcept {
    try {
        return make_pair(ImportResult::Success,
                         import(verifyBlock(&_block, m_onBad, ImportRequirements::OutOfOrderChecks), _stateDB,
                                _stateExDB, _mustBeNew));
    }
    catch (UnknownParent &) {
        return make_pair(ImportResult::UnknownParent, ImportRoute());
    }
    catch (AlreadyHaveBlock &) {
        return make_pair(ImportResult::AlreadyKnown, ImportRoute());
    }
    catch (FutureTime &) {
        return make_pair(ImportResult::FutureTimeKnown, ImportRoute());
    }
    catch (Exception &ex) {
        if (m_onBad)
            m_onBad(ex);
        return make_pair(ImportResult::Malformed, ImportRoute());
    }
}

ImportRoute BlockChain::import(bytes const &_block, OverlayDB const &_db, ex::exchange_plugin &_exdb, bool _mustBeNew) {
    // VERIFY: populates from the block and checks the block is internally coherent.
    VerifiedBlockRef const block = verifyBlock(&_block, m_onBad, ImportRequirements::OutOfOrderChecks);
    return import(block, _db, _exdb, _mustBeNew);
}

void BlockChain::insert(bytes const &_block, bytesConstRef _receipts, bool _mustBeNew) {
    // VERIFY: populates from the block and checks the block is internally coherent.
    VerifiedBlockRef const block = verifyBlock(&_block, m_onBad, ImportRequirements::OutOfOrderChecks);
    insert(block, _receipts, _mustBeNew);
}

void BlockChain::insert(VerifiedBlockRef _block, bytesConstRef _receipts, bool _mustBeNew) {
    // Check block doesn't already exist first!
    if (_mustBeNew)
        checkBlockIsNew(_block);

    // Work out its number as the parent's number + 1
    if (!isKnown(_block.info.parentHash(), false)) {
        LOG(m_logger) << _block.info.hash() << " : Unknown parent " << _block.info.parentHash();
        // We don't know the parent (yet) - discard for now. It'll get resent to us if we find out about its ancestry later on.
        BOOST_THROW_EXCEPTION(UnknownParent());
    }

    // Check receipts
    vector<bytesConstRef> receipts;
    for (auto i: RLP(_receipts))
        receipts.push_back(i.data());
    h256 receiptsRoot = orderedTrieRoot(receipts);
    if (_block.info.receiptsRoot() != receiptsRoot) {
        LOG(m_logger) << _block.info.hash() << " : Invalid receipts root "
                      << _block.info.receiptsRoot() << " not " << receiptsRoot;
        // We don't know the parent (yet) - discard for now. It'll get resent to us if we find out about its ancestry later on.
        BOOST_THROW_EXCEPTION(InvalidReceiptsStateRoot());
    }

    auto pd = details(_block.info.parentHash());
    if (!pd) {
        auto pdata = pd.rlp();
        LOG(m_loggerError) << "Details is returning false despite block known: " << RLP(pdata);
        auto parentBlock = block(_block.info.parentHash());
        LOG(m_loggerError) << "isKnown: " << isKnown(_block.info.parentHash());
        LOG(m_loggerError) << "last/number: " << m_lastBlockNumber << " " << m_lastBlockHash << " "
                           << _block.info.number();
        LOG(m_loggerError) << "Block: " << BlockHeader(&parentBlock);
        LOG(m_loggerError) << "RLP: " << RLP(parentBlock);
        LOG(m_loggerError) << "DATABASE CORRUPTION: CRITICAL FAILURE";
        exit(-1);
    }

    // Check it's not crazy
    checkBlockTimestamp(_block.info);

    // Verify parent-critical parts
    verifyBlock(_block.block, m_onBad, ImportRequirements::InOrderChecks);

    // OK - we're happy. Insert into database.
    std::unique_ptr<db::WriteBatchFace> blocksWriteBatch = m_blocksDB->createWriteBatch();
    std::unique_ptr<db::WriteBatchFace> extrasWriteBatch = m_extrasDB->createWriteBatch();

    BlockLogBlooms blb;
    for (auto i: RLP(_receipts))
        blb.blooms.push_back(TransactionReceipt(i.data()).bloom());

    // ensure parent is cached for later addition.
    // TODO: this is a bit horrible would be better refactored into an enveloping UpgradableGuard
    // together with an "ensureCachedWithUpdatableLock(l)" method.
    // This is safe in practice since the caches don't get flushed nearly often enough to be
    // done here.
    details(_block.info.parentHash());
    DEV_WRITE_GUARDED(x_details) {
        if (!dev::contains(m_details[_block.info.parentHash()].children, _block.info.hash()))
            m_details[_block.info.parentHash()].children.push_back(_block.info.hash());
    }

    blocksWriteBatch->insert(toSlice(_block.info.hash()), db::Slice(_block.block));
    DEV_READ_GUARDED(x_details)
        extrasWriteBatch->insert(toSlice(_block.info.parentHash(), ExtraDetails),
                                 (db::Slice) dev::ref(
                                         m_details[_block.info.parentHash()].rlp()));

    BlockDetails bd((unsigned) pd.number + 1, pd.totalDifficulty + _block.info.difficulty(), _block.info.parentHash(),
                    {});
    extrasWriteBatch->insert(
            toSlice(_block.info.hash(), ExtraDetails), (db::Slice) dev::ref(bd.rlp()));
    extrasWriteBatch->insert(
            toSlice(_block.info.hash(), ExtraLogBlooms), (db::Slice) dev::ref(blb.rlp()));
    extrasWriteBatch->insert(toSlice(_block.info.hash(), ExtraReceipts), (db::Slice) _receipts);

    try {
        m_blocksDB->commit(std::move(blocksWriteBatch));
    }
    catch (boost::exception const &ex) {
        cwarn << "Error writing to blockchain database: " << boost::diagnostic_information(ex);
        cwarn << "Fail writing to blockchain database. Bombing out.";
        exit(-1);
    }

    try {
        m_extrasDB->commit(std::move(extrasWriteBatch));
    }
    catch (boost::exception const &ex) {
        cwarn << "Error writing to extras database: " << boost::diagnostic_information(ex);
        cwarn << "Fail writing to extras database. Bombing out.";
        exit(-1);
    }
}

ImportRoute
BlockChain::import(VerifiedBlockRef const &_block, OverlayDB const &_db, ex::exchange_plugin &_exdb, bool _mustBeNew) {
    //@tidy This is a behemoth of a method - could do to be split into a few smaller ones.
    // Check block doesn't already exist first!
    if (_mustBeNew)
        checkBlockIsNew(_block);

    // Work out its number as the parent's number + 1
    if (!isKnown(_block.info.parentHash(), false))  // doesn't have to be current.
    {
        LOG(m_logger) << _block.info.hash() << " : Unknown parent " << _block.info.parentHash();
        // We don't know the parent (yet) - discard for now. It'll get resent to us if we find out about its ancestry later on.
        BOOST_THROW_EXCEPTION(UnknownParent() << errinfo_hash256(_block.info.parentHash()));
    }

    bool can_exe = update_cache_fork_database(_block, _db, _exdb);

    if (!can_exe) {
        return ImportRoute();
    }
    auto ret = execute_block(_block, _db, _exdb, _mustBeNew);

    std::vector<std::list<VerifiedBlockRef>> copy_data;

    DEV_READ_GUARDED(x_cached_blocks)
    for(auto &itr : m_cached_blocks){
        ///every list, max size  <= m_params.config_blocks
        if(itr.size() > m_params.config_blocks){
            itr.pop_front();
        }

        //this chain must in main chain,
        if(m_blocksDB->exists(toSlice(itr.front().info.hash()))){
            ///pop config_blocks.
            while(itr.size() > 0
            && info().number() > m_params.config_blocks
            && info().number() - itr.front().info.number() >= m_params.config_blocks ){
//                cwarn << "pop " << itr.front().info.number();
                itr.pop_front();
            }
            ///again check this chain in main chain.
            if(itr.size() > 0 && m_blocksDB->exists(toSlice(itr.front().info.hash()))){
                copy_data.push_back(itr);
            }
            else{
                cwarn << "remove " << itr.front().info.number() << "  h: " << itr.front().info.hash();
            }
        }
        else{
            cwarn << "remove " << itr.front().info.number() << "  h: " << itr.front().info.hash();
        }
    }
    DEV_WRITE_GUARDED(x_cached_blocks){
        m_cached_blocks.clear();
        DEV_WRITE_GUARDED(x_cached_blocks)
        m_cached_blocks = copy_data;
    }



    //remove unused hash and bytes.
    std::vector<h256> remove_hash;
    DEV_READ_GUARDED(x_cached_blocks)
    for(auto &itr : m_cached_bytes){
        bool find = false;
        for(auto &list : m_cached_blocks){
            for(auto &b : list){
                if(itr.first == b.info.hash()){
                    find = true;
                }
            }
        }
        if(!find){
            remove_hash.push_back(itr.first);
        }
    }

    DEV_WRITE_GUARDED(x_cached_blocks)
    for(auto &itr : remove_hash){
        m_cached_bytes.erase(itr);
    }

    auto print_route = [](const std::vector<std::list<VerifiedBlockRef>> &data) {
        for (auto itr : data) {
            std::ostringstream os;
            os << "size : " << itr.size() << "  ";
            for (auto detail : itr) {
                os << "n: " << std::to_string(detail.info.number()) << " h: " << detail.info.hash() << " p: "
                   << detail.info.parentHash() << " |";
            }
            cwarn << os.str();
        }
    };

    DEV_READ_GUARDED(x_cached_blocks)
    if(m_cached_blocks.size() > 2){
        cwarn << "config ...............";
        print_route(m_cached_blocks);
        cwarn << " map size " << m_cached_bytes.size() ;
    }
    if(info().number() > m_params.config_blocks){
        _exdb.commit_disk(info().number() - m_params.config_blocks + 1);
    }
    return  ret;
}


ImportRoute BlockChain::execute_block(const dev::brc::VerifiedBlockRef &_block, const dev::OverlayDB &_db,
                                      dev::brc::ex::exchange_plugin &_exdb, bool _mustBeNew) {
    ImportPerformanceLogger performanceLogger;
    auto pd = details(_block.info.parentHash());
    if (!pd) {
        auto pdata = pd.rlp();
        LOG(m_loggerError) << "Details is returning false despite block known: " << RLP(pdata);
        auto parentBlock = block(_block.info.parentHash());
        LOG(m_loggerError) << "isKnown: " << isKnown(_block.info.parentHash());
        LOG(m_loggerError) << "last/number: " << m_lastBlockNumber << " " << m_lastBlockHash << " "
                           << _block.info.number();
        LOG(m_loggerError) << "Block: " << BlockHeader(&parentBlock);
        LOG(m_loggerError) << "RLP: " << RLP(parentBlock);
        LOG(m_loggerError) << "DATABASE CORRUPTION: CRITICAL FAILURE";
        exit(-1);
    }


    checkBlockTimestamp(_block.info);
    // Verify parent-critical parts
    verifyBlock(_block.block, m_onBad, ImportRequirements::InOrderChecks);
    LOG(m_loggerDetail) << "Attempting import of " << _block.info.hash() << " ...";

    performanceLogger.onStageFinished("preliminaryChecks");


    BlockReceipts br;
    u256 td;
    try {
        // Check transactions are valid and that they result in a state equivalent to our state_root.
        // Get total difficulty increase and update state, checking it.
        Block s(*this, _db, _exdb);
        auto tdIncrease = s.enactOn(_block, *this);
        for (unsigned i = 0; i < s.pending().size(); ++i)
            br.receipts.push_back(s.receipt(i));
        s.cleanup();
        td = pd.totalDifficulty + tdIncrease;
        performanceLogger.onStageFinished("enactment");
#if BRC_PARANOIA
        checkConsistency();
#endif // BRC_PARANOIA
    }
    catch (BadRoot &ex) {
        cwarn << "*** BadRoot error! Trying to import" << _block.info.hash() << "needed root"
              << *boost::get_error_info<errinfo_hash256>(ex);
        cwarn << _block.info;
        // Attempt in import later.
        BOOST_THROW_EXCEPTION(TransientError());
    }
    catch (Exception &ex) {
        addBlockInfo(ex, _block.info, _block.block.toBytes());
        throw;
    }

    // All ok - insert into DB
    bytes const receipts = br.rlp();
    return insertBlockAndExtras(_block, ref(receipts), td, performanceLogger);

}


bool BlockChain::update_cache_fork_database(const dev::brc::VerifiedBlockRef &_block, const dev::OverlayDB &_db,
                                            dev::brc::ex::exchange_plugin &_exdb) {
    //block is too old.
    if (_block.info.number() < info().number() - m_params.config_blocks && info().number() > m_params.config_blocks) {
        cerror << " this block is too old , block number: " << _block.info.number() << " hash : " << _block.info.hash()
               << " , will be remove.";
        BOOST_THROW_EXCEPTION(BlockIsTooOld());
    }

    if (info().number() == 0 || m_cached_blocks.size() == 0) {
        DEV_WRITE_GUARDED(x_cached_blocks)
        m_cached_blocks.push_back({_block});
        return true;
    }

    auto print_route = [](const std::vector<std::list<VerifiedBlockRef>> &data) {
        for (auto itr : data) {
            std::ostringstream os;
            for (auto detail : itr) {
                os << "n: " << std::to_string(detail.info.number()) << " h: " << detail.info.hash() << " p: "
                   << detail.info.parentHash() << " |";
            }
            cwarn << os.str();
        }
    };
//    cwarn << "insert -----------------";
//    print_route(m_cached_blocks);
    bool find = false;
    DEV_WRITE_GUARDED(x_cached_blocks)
    for (auto &itr : m_cached_blocks) {
        for (auto &detail : itr) {
            if (_block.info.parentHash() == detail.info.hash()) {
                find = true;
                //in this branch, this block is end. only insert.
                if (detail.info.hash() == itr.back().info.hash()) {
                    itr.push_back(_block);
                } else {
                    ///copy
                    std::list<VerifiedBlockRef> copy_data;
                    for (auto cp = itr.begin(); cp != itr.end(); cp++) {
                        copy_data.push_back(*cp);
                        m_cached_bytes[cp->info.hash()] = cp->block.toBytes();
                        if (cp->info.hash() == detail.info.hash()) {
                            break;
                        }
                    }
                    copy_data.push_back(_block);
                    m_cached_blocks.push_back(copy_data);
                    cwarn << "insert block: " << _block.info.hash() << " number: " << _block.info.number();
                }
                m_cached_bytes[_block.info.hash()] = _block.block.toBytes();
                break;
            }
        }
        if (find) {
            break;
        }
    }
//    print_route(m_cached_blocks);
    if (!find) {
        cwarn << "cant find parent hash.";
        return false;
    }
    //this block is true, dont switch chain.
    if (_block.info.parentHash() == info().hash()) {
        return true;
    } else {
        ///dont switch chain, only insert this block to m_cached_blocks
        if (_block.info.number() <= info().number()) {
            cwarn << "only insert , can't switch chain.";
            return false;
        } else if (_block.info.number() == info().number() + 1) {
            /// this condition , chain must switch.
            /*  step 1, we will remove last chain  from same block's hash.
             *  step 2, delete executive blocks state and rollback exdb state until same block's hash. then execute new blocks.
             *  step 3,, execute newest block for switch new state.
             * */
            cwarn << "will switch chain, delete state.";
            auto current_hash = info().hash();
            auto insert_hash = _block.info.hash();

            std::list<VerifiedBlockRef> source_block_list;      // source blocks, will switch insert_hash(_block).
            std::list<VerifiedBlockRef> dest_block_list;        //  switch from source_block_list to  dest_block_list.

            DEV_READ_GUARDED(x_cached_blocks)
            for (const auto &itr : m_cached_blocks) {
                for (const auto &detail : itr) {
                    if (current_hash == detail.info.hash() && source_block_list.size() == 0) {
                        for (const auto &cp : itr) {
                            source_block_list.push_back(cp);
                        }
                    }
                    if (insert_hash == detail.info.hash() && dest_block_list.size() == 0) {
                        for (const auto &cp : itr) {
                            dest_block_list.push_back(cp);
                        }
                    }

                }
            }
            if (source_block_list.size() == 0 || dest_block_list.size() == 0) {
                cerror << "cant find data....., will exit.";
                exit(-1);
            }

            bool find_common_hash = false;
            h256 common_hash = Invalid256;
            VerifiedBlockRef common_block;
            for (const auto &itr : source_block_list) {
                for (const auto &to : dest_block_list) {
                    if (to.info.hash() == itr.info.hash()) {
                        find_common_hash = true;
                        common_hash = to.info.hash();
                        common_block = to;
                        break;
                    }
                }
            }

            if (!find_common_hash || common_hash == Invalid256) {
                cwarn << "cant find comment hash ...";
                //maybe, this is impossible considtion.
                return false;
            } else {
                cwarn << "find common hash : " << toHex(common_hash);
            }
            //remove is executed blocks.
            while (source_block_list.front().info.hash() != common_hash && source_block_list.size()) {
                source_block_list.pop_front();
            }

            while (dest_block_list.front().info.hash() != common_hash && dest_block_list.size()) {
                dest_block_list.pop_front();
            }

            source_block_list.pop_front();
            dest_block_list.pop_front();

            cwarn << "-------------------------------11";
            print_route({source_block_list});
            print_route({dest_block_list});
            cwarn << "-------------------------------11";
            //find state_root , and then remove them.
            remove_blocks_from_database(source_block_list, _db, _exdb);
            rollback_from_database(source_block_list.back(), common_block, source_block_list, _db, _exdb);
            m_lastBlockHash = common_hash;
            m_lastBlockNumber = common_block.info.number();
            cwarn << "current state number: " << info().number() << " state root: " << info().stateRoot() << " hash: " << info().hash();
            //switch chain, executive
            auto execute_size = dest_block_list.size() - 1;
            /// try to execute block on new chain.
            if(execute_size > 0){
                for(auto itr : dest_block_list){
                    execute_size--;
                    try {
                        DEV_READ_GUARDED(x_cached_blocks){
                            if(!m_cached_bytes.count(itr.info.hash())){
                                cwarn << "cant find block " << itr.info.hash() << " number: " << itr.info.number();
                                exit(2);
                            }
                            itr.block = bytesConstRef(&m_cached_bytes[itr.info.hash()]);
                            execute_block(itr, _db, _exdb);
                        }
                    }catch (...){
                        cerror << "fork exception :  rollback database error.";
                    }
                    if(execute_size == 0){
                        break;
                    }
                }
            }
            else{
                cerror << "dest_block_list size error  .... please check, its important.";
            }
            return true;
        } else {
            //TODO
            cerror << "unkown block ,  this block number too height..";

        }
    }

    return false;
}


uint32_t BlockChain::remove_blocks_from_database(const std::list<dev::brc::VerifiedBlockRef> &blocks,
                                                 const dev::OverlayDB &_db, dev::brc::ex::exchange_plugin &_exdb) {
    for(const auto &itr : blocks){
        const auto &remove_hash = toSlice(itr.info.hash());
        if(m_blocksDB->exists(remove_hash)){
            m_blocksDB->kill(remove_hash);
        }
        else if(m_extrasDB->exists(remove_hash)){
            m_extrasDB->kill(remove_hash);
        }
        else{
            cwarn << "remove block data error , hash : " << itr.info.hash() << " height: " << itr.info.number();
        }
    }


    return 0;
}


bool BlockChain::rollback_from_database(const dev::brc::VerifiedBlockRef &from, const dev::brc::VerifiedBlockRef &to, const std::list<dev::brc::VerifiedBlockRef> &blocks,
                                        const dev::OverlayDB &_db, dev::brc::ex::exchange_plugin &_exdb) {
    dev::OverlayDB overdb(_db);
    cwarn << "rollback block from : " << from.info.number() << "(" << from.info.hash() << ")  to : " << to.info.number() << "(" << to.info.hash() << ")";
    if(overdb.exists(from.info.stateRoot())){
        cwarn << "find from state root hash : " << toHex(from.info.stateRoot());
    }

    VerifiedBlockRef from_block = from;
    while(from_block.info.stateRoot() != to.info.stateRoot()){
        if(overdb.exists(from_block.info.stateRoot())){
            cwarn << "will remove state root " << from_block.info.stateRoot();
            overdb.kill(from_block.info.stateRoot());
        }
        else{
            cerror << "cant find from state root : " << from_block.info.stateRoot() << " number: " << from_block.info.number() << "  to: " << to.info.stateRoot() << " ("<< to.info.stateRoot() <<")";
            return false;
        }
        for(const auto &itr : blocks){
            if(itr.info.hash() == from_block.info.parentHash()){
                from_block = itr;
                break;
            }
        }
    }
    _exdb.rollback_until(to.info.hash(), to.info.stateRoot());
    return true;
}


ImportRoute
BlockChain::insertWithoutParent(bytes const &_block, bytesConstRef _receipts, u256 const &_totalDifficulty) {
    VerifiedBlockRef const block = verifyBlock(&_block, m_onBad, ImportRequirements::OutOfOrderChecks);

    // Check block doesn't already exist first!
    checkBlockIsNew(block);

    checkBlockTimestamp(block.info);

    ImportPerformanceLogger performanceLogger;
    return insertBlockAndExtras(block, _receipts, _totalDifficulty, performanceLogger);
}

void BlockChain::checkBlockIsNew(VerifiedBlockRef const &_block) const {
    if (isKnown(_block.info.hash())) {
        LOG(m_logger) << _block.info.hash() << " : Not new.";
        BOOST_THROW_EXCEPTION(AlreadyHaveBlock() << errinfo_block(_block.block.toBytes()));
    }
}

void BlockChain::checkBlockTimestamp(BlockHeader const &_header) const {
    // Check it's not crazy
    if (_header.timestamp() > utcTimeMilliSec() && !m_params.allowFutureBlocks) {
        cwarn << _header.hash() << " : Future time " << _header.timestamp()
              << " (now at " << utcTimeMilliSec() << ")";
        // Block has a timestamp in the future. This is no good.
        BOOST_THROW_EXCEPTION(FutureTime());
    }
}

ImportRoute
BlockChain::insertBlockAndExtras(VerifiedBlockRef const &_block, bytesConstRef _receipts, u256 const &_totalDifficulty,
                                 ImportPerformanceLogger &_performanceLogger) {
    std::unique_ptr<db::WriteBatchFace> blocksWriteBatch = m_blocksDB->createWriteBatch();
    std::unique_ptr<db::WriteBatchFace> extrasWriteBatch = m_extrasDB->createWriteBatch();
    h256 newLastBlockHash = currentHash();
    unsigned newLastBlockNumber = number();
    try {
        // ensure parent is cached for later addition.
        // TODO: this is a bit horrible would be better refactored into an enveloping UpgradableGuard
        // together with an "ensureCachedWithUpdatableLock(l)" method.
        // This is safe in practice since the caches don't get flushed nearly often enough to be
        // done here.
        details(_block.info.parentHash());
        DEV_WRITE_GUARDED(x_details)m_details[_block.info.parentHash()].children.push_back(_block.info.hash());

        _performanceLogger.onStageFinished("collation");

        blocksWriteBatch->insert(toSlice(_block.info.hash()), db::Slice(_block.block));
        DEV_READ_GUARDED(x_details)
            extrasWriteBatch->insert(toSlice(_block.info.parentHash(), ExtraDetails),
                                     (db::Slice) dev::ref(
                                             m_details[_block.info.parentHash()].rlp()));

        BlockDetails const details((unsigned) _block.info.number(), _totalDifficulty, _block.info.parentHash(), {});
        extrasWriteBatch->insert(
                toSlice(_block.info.hash(), ExtraDetails), (db::Slice) dev::ref(details.rlp()));

        BlockLogBlooms blb;
        for (auto i: RLP(_receipts))
            blb.blooms.push_back(TransactionReceipt(i.data()).bloom());
        extrasWriteBatch->insert(
                toSlice(_block.info.hash(), ExtraLogBlooms), (db::Slice) dev::ref(blb.rlp()));

        extrasWriteBatch->insert(toSlice(_block.info.hash(), ExtraReceipts), (db::Slice) _receipts);

        _performanceLogger.onStageFinished("writing");
    }
    catch (Exception &ex) {
        addBlockInfo(ex, _block.info, _block.block.toBytes());
        throw;
    }

    h256s route;
    h256 common;
    bool isImportedAndBest = false;
    // This might be the new best block...
    h256 last = currentHash();
    if (_totalDifficulty > details(last).totalDifficulty || (m_sealEngine->chainParams().tieBreakingGas &&
                                                             _totalDifficulty == details(last).totalDifficulty &&
                                                             _block.info.gasUsed() > info(last).gasUsed())) {
//        cwarn << "switch from " << m_lastBlockNumber << " to " << _block.info.number();
        // don't include bi.hash() in treeRoute, since it's not yet in details DB...
        // just tack it on afterwards.
        unsigned commonIndex;
        tie(route, common, commonIndex) = treeRoute(last, _block.info.parentHash());
        route.push_back(_block.info.hash());

        // Most of the time these two will be equal - only when we're doing a chain revert will they not be
        if (common != last)
            DEV_READ_GUARDED(x_lastBlockHash)clearCachesDuringChainReversion(number(common) + 1);

        // Go through ret backwards (i.e. from new head to common) until hash != last.parent and
        // update m_transactionAddresses, m_blockHashes
        for (auto i = route.rbegin(); i != route.rend() && *i != common; ++i) {
            BlockHeader tbi;
            if (*i == _block.info.hash())
                tbi = _block.info;
            else
                tbi = BlockHeader(block(*i));

            // Collate logs into blooms.
            h256s alteredBlooms;
            {
                LogBloom blockBloom = tbi.logBloom();
                blockBloom.shiftBloom<3>(sha3(tbi.author().ref()));

                // Pre-memoize everything we need before locking x_blocksBlooms
                for (unsigned level = 0, index = (unsigned) tbi.number();
                     level < c_bloomIndexLevels; level++, index /= c_bloomIndexSize)
                    blocksBlooms(chunkId(level, index / c_bloomIndexSize));

                WriteGuard l(x_blocksBlooms);
                for (unsigned level = 0, index = (unsigned) tbi.number();
                     level < c_bloomIndexLevels; level++, index /= c_bloomIndexSize) {
                    unsigned i = index / c_bloomIndexSize;
                    unsigned o = index % c_bloomIndexSize;
                    alteredBlooms.push_back(chunkId(level, i));
                    m_blocksBlooms[alteredBlooms.back()].blooms[o] |= blockBloom;
                }
            }
            // Collate transaction hashes and remember who they were.
            //h256s newTransactionAddresses;
            {
                bytes blockBytes;
                RLP blockRLP(*i == _block.info.hash() ? _block.block : &(blockBytes = block(*i)));
                TransactionAddress ta;
                ta.blockHash = tbi.hash();
                for (ta.index = 0; ta.index < blockRLP[1].itemCount(); ++ta.index)
                    extrasWriteBatch->insert(
                            toSlice(sha3(blockRLP[1][ta.index].data()), ExtraTransactionAddress),
                            (db::Slice) dev::ref(ta.rlp()));
            }

            // Update database with them.
            ReadGuard l1(x_blocksBlooms);
            for (auto const &h: alteredBlooms)
                extrasWriteBatch->insert(
                        toSlice(h, ExtraBlocksBlooms), (db::Slice) dev::ref(m_blocksBlooms[h].rlp()));
            extrasWriteBatch->insert(toSlice(h256(tbi.number()), ExtraBlockHash),
                                     (db::Slice) dev::ref(BlockHash(tbi.hash()).rlp()));
        }

        // FINALLY! change our best hash.
        {
            newLastBlockHash = _block.info.hash();
            newLastBlockNumber = (unsigned) _block.info.number();
            isImportedAndBest = true;
        }

        LOG(m_logger) << "   Imported and best " << _totalDifficulty << " (#"
                      << _block.info.number() << "). Has "
                      << (details(_block.info.parentHash()).children.size() - 1)
                      << " siblings. Route: " << route;
    } else {
        LOG(m_logger) << "   Imported but not best (oTD: " << details(last).totalDifficulty
                      << " > TD: " << _totalDifficulty << "; " << details(last).number << ".."
                      << _block.info.number() << ")";
    }

    try {
        m_blocksDB->commit(std::move(blocksWriteBatch));
    }
    catch (boost::exception &ex) {
        cwarn << "Error writing to blockchain database: " << boost::diagnostic_information(ex);
        cwarn << "Fail writing to blockchain database. Bombing out.";
        exit(-1);
    }

    try {
        m_extrasDB->commit(std::move(extrasWriteBatch));
    }
    catch (boost::exception &ex) {
        cwarn << "Error writing to extras database: " << boost::diagnostic_information(ex);
        cwarn << "Fail writing to extras database. Bombing out.";
        exit(-1);
    }

#if BRC_PARANOIA
    if (isKnown(_block.info.hash()) && !details(_block.info.hash()))
    {
        LOG(m_loggerError) << "Known block just inserted has no details.";
        LOG(m_loggerError) << "Block: " << _block.info;
        LOG(m_loggerError) << "DATABASE CORRUPTION: CRITICAL FAILURE";
        exit(-1);
    }

    try
    {
        State canary(_db, BaseState::Empty);
        canary.populateFromChain(*this, _block.info.hash());
    }
    catch (...)
    {
        LOG(m_loggerError) << "Failed to initialise State object form imported block.";
        LOG(m_loggerError) << "Block: " << _block.info;
        LOG(m_loggerError) << "DATABASE CORRUPTION: CRITICAL FAILURE";
        exit(-1);
    }
#endif // BRC_PARANOIA

    if (m_lastBlockHash != newLastBlockHash)
        DEV_WRITE_GUARDED(x_lastBlockHash) {
            m_lastBlockHash = newLastBlockHash;
            m_lastBlockNumber = newLastBlockNumber;
            try {
                m_extrasDB->insert(db::Slice("best"), db::Slice((char const *) &m_lastBlockHash, 32));
            }
            catch (boost::exception const &ex) {
                cwarn << "Error writing to extras database: " << boost::diagnostic_information(ex);
                cout << "Put" << toHex(bytesConstRef(db::Slice("best"))) << "=>"
                     << toHex(bytesConstRef(db::Slice((char const *) &m_lastBlockHash, 32)));
                cwarn << "Fail writing to extras database. Bombing out.";
                exit(-1);
            }
        }

#if BRC_PARANOIA
    checkConsistency();
#endif // BRC_PARANOIA

    _performanceLogger.onStageFinished("checkBest");

    unsigned const gasPerSecond =
            static_cast<double>(_block.info.gasUsed()) / _performanceLogger.stageDuration("enactment");
    _performanceLogger.onFinished({
                                          {"blockHash",    "\"" + _block.info.hash().abridged() + "\""},
                                          {"blockNumber",  toString(_block.info.number())},
                                          {"gasPerSecond", toString(gasPerSecond)},
                                          {"transactions", toString(_block.transactions.size())},
                                          {"gasUsed",      toString(_block.info.gasUsed())}
                                  });

    if (!route.empty())
        noteCanonChanged();

    if (isImportedAndBest && m_onBlockImport) {
        m_onBlockImport(_block.info);
    }

    h256s fresh;
    h256s dead;
    bool isOld = true;
    for (auto const &h: route)
        if (h == common)
            isOld = false;
        else if (isOld)
            dead.push_back(h);
        else
            fresh.push_back(h);
    return ImportRoute{dead, fresh, _block.transactions};
}

void BlockChain::clearBlockBlooms(unsigned _begin, unsigned _end) {
    //   ... c c c c c c c c c c C o o o o o o
    //   ...                               /=15        /=21
    // L0...| ' | ' | ' | ' | ' | ' | ' | 'b|x'x|x'x|x'e| /=11
    // L1...|   '   |   '   |   '   |   ' b | x ' x | x ' e |   /=6
    // L2...|       '       |       '   b   |   x   '   x   |   e   /=3
    // L3...|               '       b       |       x       '       e
    // model: c_bloomIndexLevels = 4, c_bloomIndexSize = 2

    //   ...                               /=15        /=21
    // L0...| ' ' ' | ' ' ' | ' ' ' | ' ' 'b|x'x'x'x|x'e' ' |
    // L1...|       '       '       '   b   |   x   '   x   '   e   '       |
    // L2...|               b               '               x               '                e              '                               |
    // model: c_bloomIndexLevels = 2, c_bloomIndexSize = 4

    // algorithm doesn't have the best memoisation coherence, but eh well...

    unsigned beginDirty = _begin;
    unsigned endDirty = _end;
    for (unsigned level = 0; level < c_bloomIndexLevels; level++, beginDirty /= c_bloomIndexSize, endDirty =
            (endDirty - 1) / c_bloomIndexSize + 1) {
        // compute earliest & latest index for each level, rebuild from previous levels.
        for (unsigned item = beginDirty; item != endDirty; ++item) {
            unsigned bunch = item / c_bloomIndexSize;
            unsigned offset = item % c_bloomIndexSize;
            auto id = chunkId(level, bunch);
            LogBloom acc;
            if (!!level) {
                // rebuild the bloom from the previous (lower) level (if there is one).
                auto lowerChunkId = chunkId(level - 1, item);
                for (auto const &bloom: blocksBlooms(lowerChunkId).blooms)
                    acc |= bloom;
            }
            blocksBlooms(id);   // make sure it has been memoized.
            m_blocksBlooms[id].blooms[offset] = acc;
        }
    }
}

void BlockChain::rescue(OverlayDB const &_db) {

    unsigned u = 1;
    while (true) {
        try {
            if (isKnown(numberHash(u)))
                u *= 2;
            else
                break;
        }
        catch (...) {
            break;
        }
    }
    unsigned l = u / 2;
    cout << "Finding last likely block number..." << endl;
    while (u - l > 1) {
        unsigned m = (u + l) / 2;
        cout << " " << m << flush;
        if (isKnown(numberHash(m)))
            l = m;
        else
            u = m;
    }
    cout << "  lowest is " << l << endl;
    for (; l > 0; --l) {
        h256 h = numberHash(l);
        cout << "Checking validity of " << l << " (" << h << ")..." << flush;
        try {
            cout << "block..." << flush;
            BlockHeader bi(block(h));
            cout << "extras..." << flush;
            details(h);
            cout << "state..." << flush;
            if (_db.exists(bi.stateRoot()))
                break;
        }
        catch (...) {}
    }
    cout << "OK." << endl;
    rewind(l);
}

void BlockChain::rewind(unsigned _newHead) {
    DEV_WRITE_GUARDED(x_lastBlockHash) {
        if (_newHead >= m_lastBlockNumber)
            return;
        clearCachesDuringChainReversion(_newHead + 1);
        m_lastBlockHash = numberHash(_newHead);
        m_lastBlockNumber = _newHead;
        try {
            m_extrasDB->insert(db::Slice("best"), db::Slice((char const *) &m_lastBlockHash, 32));
        }
        catch (boost::exception const &ex) {
            cwarn << "Error writing to extras database: " << boost::diagnostic_information(ex);
            cout << "Put" << toHex(bytesConstRef(db::Slice("best"))) << "=>"
                 << toHex(bytesConstRef(db::Slice((char const *) &m_lastBlockHash, 32)));
            cwarn << "Fail writing to extras database. Bombing out.";
            exit(-1);
        }
        noteCanonChanged();
    }
}

tuple<h256s, h256, unsigned>
BlockChain::treeRoute(h256 const &_from, h256 const &_to, bool _common, bool _pre, bool _post) const {
    if (!_from || !_to)
        return make_tuple(h256s(), h256(), 0);

    BlockDetails const fromDetails = details(_from);
    BlockDetails const toDetails = details(_to);
    // Needed to handle a special case when the parent of inserted block is not present in DB.
    if (fromDetails.isNull() || toDetails.isNull())
        return make_tuple(h256s(), h256(), 0);

    unsigned fn = fromDetails.number;
    unsigned tn = toDetails.number;
    h256s ret;
    h256 from = _from;
    while (fn > tn) {
        if (_pre)
            ret.push_back(from);
        from = details(from).parent;
        fn--;
    }

    h256s back;
    h256 to = _to;
    while (fn < tn) {
        if (_post)
            back.push_back(to);
        to = details(to).parent;
        tn--;
    }
    for (;; from = details(from).parent, to = details(to).parent) {
        if (_pre && (from != to || _common))
            ret.push_back(from);
        if (_post && (from != to || (!_pre && _common)))
            back.push_back(to);

        if (from == to)
            break;
        if (!from)
            assert(from);
        if (!to)
            assert(to);
    }
    ret.reserve(ret.size() + back.size());
    unsigned i = ret.size() - (int) (_common && !ret.empty() && !back.empty());
    for (auto it = back.rbegin(); it != back.rend(); ++it)
        ret.push_back(*it);
    return make_tuple(ret, from, i);
}

void BlockChain::noteUsed(h256 const &_h, unsigned _extra) const {
    auto id = CacheID(_h, _extra);
    Guard l(x_cacheUsage);
    m_cacheUsage[0].insert(id);
    if (m_cacheUsage[1].count(id))
        m_cacheUsage[1].erase(id);
    else
        m_inUse.insert(id);
}

template<class K, class T>
static unsigned getHashSize(unordered_map<K, T> const &_map) {
    unsigned ret = 0;
    for (auto const &i: _map)
        ret += i.second.size + 64;
    return ret;
}

void BlockChain::updateStats() const {
    m_lastStats.memBlocks = 0;
    DEV_READ_GUARDED(x_blocks)
        for (auto const &i: m_blocks)
            m_lastStats.memBlocks += i.second.size() + 64;
    DEV_READ_GUARDED(x_details)m_lastStats.memDetails = getHashSize(m_details);
    size_t logBloomsSize = 0;
    size_t blocksBloomsSize = 0;
    DEV_READ_GUARDED(x_logBlooms)logBloomsSize = getHashSize(m_logBlooms);
    DEV_READ_GUARDED(x_blocksBlooms)blocksBloomsSize = getHashSize(m_blocksBlooms);
    m_lastStats.memLogBlooms = logBloomsSize + blocksBloomsSize;
    DEV_READ_GUARDED(x_receipts)m_lastStats.memReceipts = getHashSize(m_receipts);
    DEV_READ_GUARDED(x_blockHashes)m_lastStats.memBlockHashes = getHashSize(m_blockHashes);
    DEV_READ_GUARDED(x_transactionAddresses)m_lastStats.memTransactionAddresses = getHashSize(m_transactionAddresses);
}

void BlockChain::garbageCollect(bool _force) {
    updateStats();

    if (!_force && chrono::system_clock::now() < m_lastCollection + c_collectionDuration &&
        m_lastStats.memTotal() < c_maxCacheSize)
        return;
    if (m_lastStats.memTotal() < c_minCacheSize)
        return;

    m_lastCollection = chrono::system_clock::now();

    Guard l(x_cacheUsage);
    for (CacheID const &id: m_cacheUsage.back()) {
        m_inUse.erase(id);
        // kill i from cache.
        switch (id.second) {
            case (unsigned) -1: {
                WriteGuard l(x_blocks);
                m_blocks.erase(id.first);
                break;
            }
            case ExtraDetails: {
                WriteGuard l(x_details);
                m_details.erase(id.first);
                break;
            }
            case ExtraBlockHash: {
                // m_cacheUsage should not contain ExtraBlockHash elements currently.  See the second noteUsed() in BlockChain.h, which is a no-op.
                assert(false);
                break;
            }
            case ExtraReceipts: {
                WriteGuard l(x_receipts);
                m_receipts.erase(id.first);
                break;
            }
            case ExtraLogBlooms: {
                WriteGuard l(x_logBlooms);
                m_logBlooms.erase(id.first);
                break;
            }
            case ExtraTransactionAddress: {
                WriteGuard l(x_transactionAddresses);
                m_transactionAddresses.erase(id.first);
                break;
            }
            case ExtraBlocksBlooms: {
                WriteGuard l(x_blocksBlooms);
                m_blocksBlooms.erase(id.first);
                break;
            }
        }
    }
    m_cacheUsage.pop_back();
    m_cacheUsage.push_front(std::unordered_set<CacheID>{});
}

void BlockChain::checkConsistency() {
    DEV_WRITE_GUARDED(x_details) { m_details.clear(); }

    m_blocksDB->forEach([this](db::Slice const &_key, db::Slice const & /* _value */) {
        if (_key.size() == 32) {
            h256 h((byte const *) _key.data(), h256::ConstructFromPointer);
            auto dh = details(h);
            auto p = dh.parent;
            if (p != h256() && p != m_genesisHash)  // TODO: for some reason the genesis details
                // with the children get squished. not sure
                // why.
            {
                auto dp = details(p);
                if (asserts(contains(dp.children, h)))
                    cnote << "Apparently the database is corrupt. Not much we can do at this "
                             "stage...";
                if (assertsEqual(dp.number, dh.number - 1))
                    cnote << "Apparently the database is corrupt. Not much we can do at this "
                             "stage...";
            }
        }
        return true;
    });
}

void BlockChain::clearCachesDuringChainReversion(unsigned _firstInvalid) {
    unsigned end = m_lastBlockNumber + 1;
    DEV_WRITE_GUARDED(x_blockHashes)
        for (auto i = _firstInvalid; i < end; ++i)
            m_blockHashes.erase(i);
    DEV_WRITE_GUARDED(
            x_transactionAddresses)m_transactionAddresses.clear(); // TODO: could perhaps delete them individually?

    // If we are reverting previous blocks, we need to clear their blooms (in particular, to
    // rebuild any higher level blooms that they contributed to).
    clearBlockBlooms(_firstInvalid, end);
}

static inline unsigned upow(unsigned a, unsigned b) {
    if (!b) return 1;
    while (--b > 0) a *= a;
    return a;
}

static inline unsigned ceilDiv(unsigned n, unsigned d) { return (n + d - 1) / d; }
//static inline unsigned floorDivPow(unsigned n, unsigned a, unsigned b) { return n / upow(a, b); }
//static inline unsigned ceilDivPow(unsigned n, unsigned a, unsigned b) { return ceilDiv(n, upow(a, b)); }

// Level 1
// [xxx.            ]

// Level 0
// [.x............F.]
// [........x.......]
// [T.............x.]
// [............    ]

// F = 14. T = 32

vector<unsigned> BlockChain::withBlockBloom(LogBloom const &_b, unsigned _earliest, unsigned _latest) const {
    vector<unsigned> ret;

    // start from the top-level
    unsigned u = upow(c_bloomIndexSize, c_bloomIndexLevels);

    // run through each of the top-level blockbloom blocks
    for (unsigned index = _earliest / u; index <= ceilDiv(_latest, u); ++index)             // 0
        ret += withBlockBloom(_b, _earliest, _latest, c_bloomIndexLevels - 1, index);

    return ret;
}

vector<unsigned> BlockChain::withBlockBloom(LogBloom const &_b, unsigned _earliest, unsigned _latest, unsigned _level,
                                            unsigned _index) const {
    // 14, 32, 1, 0
    // 14, 32, 0, 0
    // 14, 32, 0, 1
    // 14, 32, 0, 2

    vector<unsigned> ret;

    unsigned uCourse = upow(c_bloomIndexSize, _level + 1);
    // 256
    // 16
    unsigned uFine = upow(c_bloomIndexSize, _level);
    // 16
    // 1

    unsigned obegin = _index == _earliest / uCourse ? _earliest / uFine % c_bloomIndexSize : 0;
    // 0
    // 14
    // 0
    // 0
    unsigned oend = _index == _latest / uCourse ? (_latest / uFine) % c_bloomIndexSize + 1 : c_bloomIndexSize;
    // 3
    // 16
    // 16
    // 1

    BlocksBlooms bb = blocksBlooms(_level, _index);
    for (unsigned o = obegin; o < oend; ++o)
        if (bb.blooms[o].contains(_b)) {
            // This level has something like what we want.
            if (_level > 0)
                ret += withBlockBloom(_b, _earliest, _latest, _level - 1, o + _index * c_bloomIndexSize);
            else
                ret.push_back(o + _index * c_bloomIndexSize);
        }
    return ret;
}

h256Hash BlockChain::allKinFrom(h256 const &_parent, unsigned _generations) const {
    // Get all uncles cited given a parent (i.e. featured as uncles/main in parent, parent + 1, ... parent + 5).
    h256 p = _parent;
    h256Hash ret = {p};
    // p and (details(p).parent: i == 5) is likely to be overkill, but can't hurt to be cautious.
    for (unsigned i = 0; i < _generations && p != m_genesisHash; ++i, p = details(p).parent) {
        ret.insert(details(p).parent);
        auto b = block(p);
        for (auto i: RLP(b)[2])
            ret.insert(sha3(i.data()));
    }
    return ret;
}

bool BlockChain::isKnown(h256 const &_hash, bool _isCurrent) const {
    if (_hash == m_genesisHash)
        return true;

    //
    DEV_READ_GUARDED(x_cached_blocks)
    for (const auto &itr : m_cached_blocks) {
        for (const auto &detail : itr) {
            if (detail.info.hash() == _hash) {
                return true;
            }
        }
    }


    DEV_READ_GUARDED(x_blocks)
        if (!m_blocks.count(_hash) && !m_blocksDB->exists(toSlice(_hash))) {
            return false;
        }
    DEV_READ_GUARDED(x_details)
        if (!m_details.count(_hash) && !m_extrasDB->exists(toSlice(_hash, ExtraDetails))) {
            return false;
        }
//  return true;
    return !_isCurrent || details(_hash).number <= m_lastBlockNumber;       // to allow rewind functionality.
}

bytes BlockChain::block(h256 const &_hash) const {
    if (_hash == m_genesisHash)
        return m_params.genesisBlock();

    {
        ReadGuard l(x_blocks);
        auto it = m_blocks.find(_hash);
        if (it != m_blocks.end())
            return it->second;
    }

    string const d = m_blocksDB->lookup(toSlice(_hash));
    if (d.empty()) {
        cwarn << "Couldn't find requested block:" << _hash;
        return bytes();
    }

    noteUsed(_hash);

    WriteGuard l(x_blocks);
    m_blocks[_hash].resize(d.size());
    memcpy(m_blocks[_hash].data(), d.data(), d.size());

    return m_blocks[_hash];
}

bytes BlockChain::headerData(h256 const &_hash) const {
    if (_hash == m_genesisHash)
        return m_genesisHeaderBytes;

    {
        ReadGuard l(x_blocks);
        auto it = m_blocks.find(_hash);
        if (it != m_blocks.end())
            return BlockHeader::extractHeader(&it->second).data().toBytes();
    }

    string const d = m_blocksDB->lookup(toSlice(_hash));
    if (d.empty()) {
        cwarn << "Couldn't find requested block:" << _hash;
        return bytes();
    }

    noteUsed(_hash);

    WriteGuard l(x_blocks);
    m_blocks[_hash].resize(d.size());
    memcpy(m_blocks[_hash].data(), d.data(), d.size());

    return BlockHeader::extractHeader(&m_blocks[_hash]).data().toBytes();
}

Block BlockChain::genesisBlock(OverlayDB const &_db, ex::exchange_plugin const &_exdb) const {
    h256 r = BlockHeader(m_params.genesisBlock()).stateRoot();
    Block ret(*this, _db, _exdb, BaseState::Empty);
    if (!_db.exists(r)) {
        ret.noteChain(*this);
        dev::brc::commit(m_params.genesisState,
                         ret.mutableState().m_state);        // bit horrible. maybe consider a better way of constructing it?
        ret.mutableState().systemPendingorder(ret.info().timestamp());
        ret.mutableState().db().commit();
        // have to use this db() since it's the one that has been altered with the above commit.
        if (ret.mutableState().rootHash() != r) {
            cwarn << "Hinted genesis block's state root hash is incorrect!";
            cwarn << "Hinted" << r << ", computed" << ret.mutableState().rootHash();
            // TODO: maybe try to fix it by altering the m_params's genesis block?
            exit(-1);
        }

    }
    ret.m_previousBlock = BlockHeader(m_params.genesisBlock());
    ret.resetCurrent();
    return ret;
}

VerifiedBlockRef BlockChain::verifyBlock(bytesConstRef _block, std::function<void(Exception &)> const &_onBad,
                                         ImportRequirements::value _ir) const {
    VerifiedBlockRef res;
    BlockHeader h;
    try {
        h = BlockHeader(_block);
        if (!!(_ir & ImportRequirements::PostGenesis) && (!h.parentHash() || h.number() == 0))
            BOOST_THROW_EXCEPTION(
                    InvalidParentHash() << errinfo_required_h256(h.parentHash()) << errinfo_currentNumber(h.number()));

        BlockHeader parent;
        if (!!(_ir & ImportRequirements::Parent)) {
            bytes parentHeader(headerData(h.parentHash()));
            if (parentHeader.empty())
                BOOST_THROW_EXCEPTION(InvalidParentHash() << errinfo_required_h256(h.parentHash())
                                                          << errinfo_currentNumber(h.number()));
            parent = BlockHeader(parentHeader, HeaderData, h.parentHash());
        }
        sealEngine()->verify(
                (_ir & ImportRequirements::ValidSeal) ? Strictness::CheckEverything : Strictness::QuickNonce, h, parent,
                _block);
        res.info = h;
    }
    catch (Exception &ex) {
        ex << errinfo_phase(1);
        addBlockInfo(ex, h, _block.toBytes());
        if (_onBad)
            _onBad(ex);
        throw;
    }

    RLP r(_block);
    unsigned i = 0;
    if (_ir & (ImportRequirements::UncleBasic | ImportRequirements::UncleParent | ImportRequirements::UncleSeals))
        for (auto const &uncle: r[2]) {
            BlockHeader uh(uncle.data(), HeaderData);
            try {
                BlockHeader parent;
                if (!!(_ir & ImportRequirements::UncleParent)) {
                    bytes parentHeader(headerData(uh.parentHash()));
                    if (parentHeader.empty())
                        BOOST_THROW_EXCEPTION(InvalidUncleParentHash() << errinfo_required_h256(uh.parentHash())
                                                                       << errinfo_currentNumber(h.number())
                                                                       << errinfo_uncleNumber(uh.number()));
                    parent = BlockHeader(parentHeader, HeaderData, uh.parentHash());
                }
                sealEngine()->verify(
                        (_ir & ImportRequirements::UncleSeals) ? Strictness::CheckEverything : Strictness::IgnoreSeal,
                        uh, parent);
            }
            catch (Exception &ex) {
                ex << errinfo_phase(1);
                ex << errinfo_uncleIndex(i);
                addBlockInfo(ex, uh, _block.toBytes());
                if (_onBad)
                    _onBad(ex);
                throw;
            }
            ++i;
        }
    i = 0;
    if (_ir & (ImportRequirements::TransactionBasic | ImportRequirements::TransactionSignatures)) {

        if (r[1].itemCount() > c_maxSyncTransactions) {
            BOOST_THROW_EXCEPTION(TooMuchTransaction()
                                          << errinfo_transactionIndex(r[1].itemCount())
                                          << errinfo_block(_block.toBytes()));
        }

        if (r[1].itemCount() > 300) {
            // more thread to do
            size_t th_num = r[1].itemCount() / 200 + 1;   //test for : the transaction num:150 for one thread todo
            th_num = (th_num % 2 != 0) ? th_num - 1 : th_num;
            th_num = th_num > 8 ? 8 : th_num;
            //th_num = 6;
            std::vector<bytes> v_trxb;
            for (auto val : r[1])
                v_trxb.push_back(val.data().toBytes());
            std::vector<Transaction> ret_t;
            Task<bytes, Transaction> task_t(th_num);

            try {
                task_t.go_task(v_trxb, ret_t, [&_ir](bytes const &b) {
                    return Transaction(b,
                                       (_ir & ImportRequirements::TransactionSignatures) ? CheckTransaction::Everything
                                                                                         : CheckTransaction::None);
                });
            }
            catch (Exception &ex) {
                ex << errinfo_phase(1);
                ex << errinfo_transactionIndex(i);
                addBlockInfo(ex, h, _block.toBytes());
                if (_onBad)
                    _onBad(ex);
                throw;
            }
            res.transactions.clear();
            res.transactions = ret_t;
        } else {
            double p_time = 0;
            double v_time = 0;
            for (RLP const &tr : r[1]) {
                bytesConstRef d = tr.data();
                try {
                    Transaction t(d, (_ir & ImportRequirements::TransactionSignatures) ? CheckTransaction::Everything
                                                                                       : CheckTransaction::None);
                    m_sealEngine->verifyTransaction(_ir, t, h,
                                                    0); // the gasUsed vs blockGasLimit is checked later in enact function
                    res.transactions.push_back(t);
                }
                catch (Exception &ex) {
                    ex << errinfo_phase(1);
                    ex << errinfo_transactionIndex(i);
                    ex << errinfo_transaction(d.toBytes());
                    addBlockInfo(ex, h, _block.toBytes());
                    if (_onBad)
                        _onBad(ex);
                    throw;
                }
                ++i;
            }
        }

    }
    res.block = bytesConstRef(_block);
    return res;
}

void BlockChain::setChainStartBlockNumber(unsigned _number) {
    h256 const hash = numberHash(_number);
    if (!hash)
        BOOST_THROW_EXCEPTION(UnknownBlockNumber());

    try {
        m_extrasDB->insert(
                c_sliceChainStart, db::Slice(reinterpret_cast<char const *>(hash.data()), h256::size));
    }
    catch (boost::exception const &ex) {
        BOOST_THROW_EXCEPTION(FailedToWriteChainStart()
                                      << errinfo_hash256(hash)
                                      << boost::errinfo_nested_exception(boost::copy_exception(ex)));
    }
}

unsigned BlockChain::chainStartBlockNumber() const {
    auto const value = m_extrasDB->lookup(c_sliceChainStart);
    return value.empty() ? 0 : number(h256(value, h256::FromBinary));
}
