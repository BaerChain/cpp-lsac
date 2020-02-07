#include "State.h"
#include "ExdbState.h"
#include "Block.h"
#include "BlockChain.h"
#include "DposVote.h"
#include "ExtVM.h"
#include "TransactionQueue.h"
#include <libbvm/VMFactory.h>
#include <libdevcore/Assertions.h>
#include <libdevcore/CommonJS.h>
#include <libdevcore/DBFactory.h>
#include <libdevcore/TrieHash.h>
#include <boost/filesystem.hpp>
#include <boost/timer.hpp>
#include <libbrccore/changeVote.h>

using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace dev::brc::ex;

namespace fs = boost::filesystem;

#define BRCNUM 1000
#define COOKIENUM 100000000000
#define LIMITE_NUMBER 50


State::State(u256 const &_accountStartNonce, OverlayDB const &_db, ex::exchange_plugin const &_exdb,
             BaseState _bs)
        : m_db(_db), m_exdb(_exdb), m_state(&m_db), m_accountStartNonce(_accountStartNonce) {
    if (_bs != BaseState::PreExisting)
        // Initialise to the state entailed by the genesis block; this guarantees the trie is built
        // correctly.
        m_state.init();
}

State::State(State const &_s)
        : m_db(_s.m_db),
          m_exdb(_s.m_exdb),
          m_state(&m_db, _s.m_state.root(), Verification::Skip),
          m_cache(_s.m_cache),
          m_unchangedCacheEntries(_s.m_unchangedCacheEntries),
          m_nonExistingAccountsCache(_s.m_nonExistingAccountsCache),
          m_touched(_s.m_touched),
          m_accountStartNonce(_s.m_accountStartNonce),
          m_block_number(_s.m_block_number)
          {}

OverlayDB State::openDB(fs::path const &_basePath, h256 const &_genesisHash, WithExisting _we) {
    fs::path path = _basePath.empty() ? db::databasePath() : _basePath;

    if (db::isDiskDatabase() && _we == WithExisting::Kill) {
        clog(VerbosityDebug, "statedb") << "Killing state database (WithExisting::Kill).";
        cwarn << "will remove " << path << "/" << fs::path("state");
        fs::remove_all(path / fs::path("state"));
    }

    path /=
            fs::path(toHex(_genesisHash.ref().cropped(0, 4))) / fs::path(toString(c_databaseVersion));
    if (db::isDiskDatabase()) {
        fs::create_directories(path);
        DEV_IGNORE_EXCEPTIONS(fs::permissions(path, fs::owner_all));
    }

    try {
        std::unique_ptr<db::DatabaseFace> db = db::DBFactory::create(path / fs::path("state"));
        clog(VerbosityTrace, "statedb") << "Opened state DB.";
        return OverlayDB(std::move(db));
    }
    catch (boost::exception const &ex) {
        cwarn << boost::diagnostic_information(ex) << '\n';
        if (!db::isDiskDatabase())
            throw;
        else if (fs::space(path / fs::path("state")).available < 1024) {
            cwarn << "Not enough available space found on hard drive. Please free some up and then "
                     "re-run. Bailing.";
            BOOST_THROW_EXCEPTION(NotEnoughAvailableSpace());
        } else {
            cwarn << "Database " << (path / fs::path("state"))
                  << "already open. You appear to have another instance of brcdChain running. "
                     "Bailing.";
            BOOST_THROW_EXCEPTION(DatabaseAlreadyOpen());
        }
    }
}

ex::exchange_plugin State::openExdb(boost::filesystem::path const &_path, WithExisting _we) {
    clog(VerbosityDebug, "exdb") << "Killing state database (WithExisting::Kill).";
    fs::remove_all(_path);
    try {
        ex::exchange_plugin exdb = ex::exchange_plugin(_path);
        return exdb;
    }
    catch (const std::exception &) {
        cerror << "open exDB error";
        exit(1);
    }
}

void State::populateFrom(AccountMap const &_map) {
    auto it = _map.find(Address("0xffff19f5ada6a28821ce0ed74c605c8c086ceb35"));
    Account a;
    if (it != m_cache.end())
        a = it->second;
    cerror << "State::populateFrom ";
    brc::commit(_map, m_state, blockNumber());
    commit(State::CommitBehaviour::KeepEmptyAccounts);
}

u256 const &State::requireAccountStartNonce() const {
    if (m_accountStartNonce == Invalid256)
        BOOST_THROW_EXCEPTION(InvalidAccountStartNonceInState());
    return m_accountStartNonce;
}

void State::noteAccountStartNonce(u256 const &_actual) {
    if (m_accountStartNonce == Invalid256)
        m_accountStartNonce = _actual;
    else if (m_accountStartNonce != _actual)
        BOOST_THROW_EXCEPTION(IncorrectAccountStartNonceInState());
}

void State::removeEmptyAccounts() {
    for (auto &i : m_cache)
        if (i.second.isDirty() && i.second.isEmpty())
            i.second.kill();
}

State &State::operator=(State const &_s) {
    if (&_s == this)
        return *this;
    m_db = _s.m_db;
    m_exdb = _s.m_exdb;
    m_state.open(&m_db, _s.m_state.root(), Verification::Skip);
    m_cache = _s.m_cache;
    m_unchangedCacheEntries = _s.m_unchangedCacheEntries;
    m_nonExistingAccountsCache = _s.m_nonExistingAccountsCache;
    m_touched = _s.m_touched;
    m_accountStartNonce = _s.m_accountStartNonce;
    m_block_number = _s.m_block_number;
    return *this;
}

Account const *State::account(Address const &_a) const {
    return const_cast<State *>(this)->account(_a);
}

Account *State::account(Address const &_addr) {
    auto it = m_cache.find(_addr);
    if (it != m_cache.end())
        return &it->second;

    if (m_nonExistingAccountsCache.count(_addr))
        return nullptr;

    // Populate basic info.
    string stateBack = m_state.at(_addr);
    if (stateBack.empty()) {
        m_nonExistingAccountsCache.insert(_addr);
        return nullptr;
    }

    clearCacheIfTooLarge();

    RLP state(stateBack);

    std::vector<PollData> _vote;
    for (auto val : state[6].toVector<bytes>()) {
        PollData p_data;
        p_data.populate(val);
        _vote.push_back(p_data);
    }

    const bytes _bBlockReward = state[10].toBytes();
    RLP _rlpBlockReward(_bBlockReward);
    size_t num = _rlpBlockReward[0].toInt<size_t>();
    std::vector<std::pair<u256, u256>> _blockReward;
    for (size_t k = 1; k <= num; k++) {
        std::pair<u256, u256> _blockpair = _rlpBlockReward[k].toPair<u256, u256>();
        _blockReward.push_back(_blockpair);
    }

    auto i = m_cache.emplace(std::piecewise_construct, std::forward_as_tuple(_addr),
                             std::forward_as_tuple(state[0].toInt<u256>(), state[1].toInt<u256>(),
                                                   state[2].toHash<h256>(), state[3].toHash<h256>(),
                                                   state[4].toInt<u256>(),
                                                   state[5].toInt<u256>(), state[7].toInt<u256>(),
                                                   state[8].toInt<u256>(),
                                                   state[9].toInt<u256>(),
                                                   Account::Unchanged));
    i.first->second.set_vote_data(_vote);
    i.first->second.setBlockReward(_blockReward);

    std::vector<std::string> tmp;
    tmp = state[11].convert<std::vector<std::string>>(RLP::LaissezFaire);
    i.first->second.initChangeList(tmp);

    u256 _cookieNum = state[12].toInt<u256>();
    i.first->second.setCookieIncome(_cookieNum);

    const bytes vote_sna_b = state[13].convert<bytes>(RLP::LaissezFaire);
    i.first->second.init_vote_snapshot(vote_sna_b);

    const bytes _feeSnapshotBytes = state[14].convert<bytes>(RLP::LaissezFaire);
    i.first->second.initCoupingSystemFee(_feeSnapshotBytes);

    const bytes record_b = state[15].convert<bytes>(RLP::LaissezFaire);
    i.first->second.init_block_record(record_b);

    const bytes received_cookies = state[16].convert<bytes>(RLP::LaissezFaire);
    i.first->second.init_received_cookies(received_cookies);
    m_unchangedCacheEntries.push_back(_addr);

    /// successExchange
    const bytes ret_order_b = state[17].convert<bytes>(RLP::LaissezFaire);
    i.first->second.initResultOrder(ret_order_b);
    /// ex_order
    const bytes ex_order_b = state[18].convert<bytes>(RLP::LaissezFaire);
    i.first->second.initExOrder(ex_order_b);

    if(m_block_number >= config::newChangeHeight()){
        if(state.itemCount() > 19) {
            const bytes _b = state[19].convert<bytes>(RLP::LaissezFaire);
            i.first->second.populateChangeMiner(_b);
        }
    }
    if(m_block_number >= config::changeExchange()){
        if(state.itemCount() > 20) {
            const h256 storageByteRoot = state[20].toHash<h256>();
            i.first->second.setStorageBytesRoot(storageByteRoot);
        }
    }
    return &i.first->second;
}

void State::clearCacheIfTooLarge() const {
    // TODO: Find a good magic number
    while (m_unchangedCacheEntries.size() > 1000) {
        // Remove a random element
        // FIXME: Do not use random device as the engine. The random device should be only used to
        // seed other engine.
        size_t const randomIndex = std::uniform_int_distribution<size_t>(
                0, m_unchangedCacheEntries.size() - 1)(dev::s_fixedHashEngine);

        Address const addr = m_unchangedCacheEntries[randomIndex];
        swap(m_unchangedCacheEntries[randomIndex], m_unchangedCacheEntries.back());
        m_unchangedCacheEntries.pop_back();

        auto cacheEntry = m_cache.find(addr);
        if (cacheEntry != m_cache.end() && !cacheEntry->second.isDirty())
            m_cache.erase(cacheEntry);
    }
}

void State::commit(CommitBehaviour _commitBehaviour) {
    if (_commitBehaviour == CommitBehaviour::RemoveEmptyAccounts)
        removeEmptyAccounts();
    m_touched += dev::brc::commit(m_cache, m_state, blockNumber());
    m_changeLog.clear();
    m_cache.clear();
    m_unchangedCacheEntries.clear();
}

unordered_map<Address, u256> State::addresses() const {
#if BRC_FATDB
    unordered_map<Address, u256> ret;
    for (auto& i : m_cache)
        if (i.second.isAlive())
            ret[i.first] = i.second.balance();
    for (auto const& i : m_state)
        if (m_cache.find(i.first) == m_cache.end())
            ret[i.first] = RLP(i.second)[1].toInt<u256>();
    return ret;
#else
    BOOST_THROW_EXCEPTION(InterfaceNotSupported() << errinfo_interface("State::addresses()"));
#endif
}

std::pair<State::AddressMap, h256> State::addresses(
        h256 const &_beginHash, size_t _maxResults) const {
    AddressMap addresses;
    h256 nextKey;

#if BRC_FATDB
    for (auto it = m_state.hashedLowerBound(_beginHash); it != m_state.hashedEnd(); ++it)
    {
        auto const address = Address(it.key());
        auto const itCachedAddress = m_cache.find(address);

        // skip if deleted in cache
        if (itCachedAddress != m_cache.end() && itCachedAddress->second.isDirty() &&
            !itCachedAddress->second.isAlive())
            continue;

        // break when _maxResults fetched
        if (addresses.size() == _maxResults)
        {
            nextKey = h256((*it).first);
            break;
        }

        h256 const hashedAddress((*it).first);
        addresses[hashedAddress] = address;
    }
#endif

    // get addresses from cache with hash >= _beginHash (both new and old touched, we can't
    // distinguish them) and order by hash
    AddressMap cacheAddresses;
    for (auto const &addressAndAccount : m_cache) {
        auto const &address = addressAndAccount.first;
        auto const addressHash = sha3(address);
        auto const &account = addressAndAccount.second;
        if (account.isDirty() && account.isAlive() && addressHash >= _beginHash)
            cacheAddresses.emplace(addressHash, address);
    }

    // merge addresses from DB and addresses from cache
    addresses.insert(cacheAddresses.begin(), cacheAddresses.end());

    // if some new accounts were created in cache we need to return fewer results
    if (addresses.size() > _maxResults) {
        auto itEnd = std::next(addresses.begin(), _maxResults);
        nextKey = itEnd->first;
        addresses.erase(itEnd, addresses.end());
    }

    return {addresses, nextKey};
}


void State::setRoot(h256 const &_r) {
    m_cache.clear();
    m_unchangedCacheEntries.clear();
    m_nonExistingAccountsCache.clear();
    //  m_touched.clear();
    m_state.setRoot(_r);
}

bool State::addressInUse(Address const &_id) const {
    return !!account(_id);
}

bool State::accountNonemptyAndExisting(Address const &_address) const {
    if (Account const *a = account(_address))
        return !a->isEmpty();
    else
        return false;
}

bool State::addressHasCode(Address const &_id) const {
    if (auto a = account(_id))
        return a->codeHash() != EmptySHA3;
    else
        return false;
}

u256 State::balance(Address const &_id) const {
    if (auto a = account(_id))
        return a->balance();
    else
        return 0;
}

u256 State::ballot(Address const &_id) const {
    if (auto a = account(_id)) {
        return a->ballot();
    } else
        return 0;
}

void State::incNonce(Address const &_addr) {
    if (Account *a = account(_addr)) {
        auto oldNonce = a->nonce();
        a->incNonce();
        m_changeLog.emplace_back(_addr, oldNonce);
    } else
        // This is possible if a transaction has gas price 0.
        createAccount(_addr, Account(requireAccountStartNonce() + 1, 0));
}

void State::setNonce(Address const &_addr, u256 const &_newNonce) {
    if (Account *a = account(_addr)) {
        auto oldNonce = a->nonce();
        a->setNonce(_newNonce);
        m_changeLog.emplace_back(_addr, oldNonce);
    } else
        // This is possible when a contract is being created.
        createAccount(_addr, Account(_newNonce, 0));
}

void State::addBalance(Address const &_id, u256 const &_amount) {
    if (Account *a = account(_id)) {
        // Log empty account being touched. Empty touched accounts are cleared
        // after the transaction, so this event must be also reverted.
        // We only log the first touch (not dirty yet), and only for empty
        // accounts, as other accounts does not matter.
        // TODO: to save space we can combine this event with Balance by having
        //       Balance and Balance+Touch events.
        if (!a->isDirty() && a->isEmpty())
            m_changeLog.emplace_back(Change::Touch, _id);

        // Increase the account balance. This also is done for value 0 to mark
        // the account as dirty. Dirty account are not removed from the cache
        // and are cleared if empty at the end of the transaction.
        a->addBalance(_amount);
    } else
        createAccount(_id, {requireAccountStartNonce(), _amount});

    if (_amount)
        m_changeLog.emplace_back(Change::Balance, _id, _amount);
}

void State::addBallot(Address const &_id, u256 const &_amount) {
    if (Account *a = account(_id)) {
        if (!a->isDirty() && a->isEmpty())
            m_changeLog.emplace_back(Change::Touch, _id);
        a->addBallot(_amount);
    } else
        BOOST_THROW_EXCEPTION(InvalidAddress() << errinfo_interface("State::addBallot()"));
    // createAccount(_id, {requireAccountStartNonce(), _amount});

    if (_amount)
        m_changeLog.emplace_back(Change::Ballot, _id, _amount);
}

void State::subBalance(Address const &_addr, u256 const &_value) {
    if (_value == 0)
        return;

    Account *a = account(_addr);
    if (!a || a->balance() < _value)
        // TODO: I expect this never happens.
        BOOST_THROW_EXCEPTION(NotEnoughCash());

    // Fall back to addBalance().
    addBalance(_addr, 0 - _value);
}

void State::subBallot(Address const &_addr, u256 const &_value) {
    if (_value == 0)
        return;

    Account *a = account(_addr);
    if (!a || a->ballot() < _value)
        BOOST_THROW_EXCEPTION(NotEnoughBallot());

    addBallot(_addr, 0 - _value);
}

void State::setBalance(Address const &_addr, u256 const &_value) {
    Account *a = account(_addr);
    u256 original = a ? a->balance() : 0;

    // Fall back to addBalance().
    addBalance(_addr, _value - original);
}

// BRC接口实现
u256 State::BRC(Address const &_id) const {
    if (auto *a = account(_id)) {
        return a->BRC();
    } else {
        return 0;
    }
}

void State::addBRC(Address const &_addr, u256 const &_value) {
    if (Account *a = account(_addr)) {
        if (!a->isDirty() && a->isEmpty())
            m_changeLog.emplace_back(Change::Touch, _addr);
        a->addBRC(_value);
    } else
        createAccount(_addr, {requireAccountStartNonce(), 0, _value});

    if (_value)
        m_changeLog.emplace_back(Change::BRC, _addr, _value);
}

void State::subBRC(Address const &_addr, u256 const &_value) {
    if (_value == 0)
        return;

    Account *a = account(_addr);
    if (!a || a->BRC() < _value) {
        cwarn << "_addr: " << _addr << " value " << _value << "  : " << a->BRC();
        // TODO: I expect this never happens.
        BOOST_THROW_EXCEPTION(NotEnoughCash());
    }


    // Fall back to addBalance().
    addBRC(_addr, 0 - _value);
}

void State::setBRC(Address const &_addr, u256 const &_value) {
    Account *a = account(_addr);
    u256 original = a ? a->BRC() : 0;

    // Fall back to addBalance().
    addBRC(_addr, _value - original);
}

// FBRC 相关接口实现

u256 State::FBRC(Address const &_id) const {
    if (auto a = account(_id)) {
        return a->FBRC();
    } else {
        return 0;
    }
}


void State::addFBRC(Address const &_addr, u256 const &_value) {
    if (Account *a = account(_addr)) {
        if (!a->isDirty() && a->isEmpty())
            m_changeLog.emplace_back(Change::Touch, _addr);
        a->addFBRC(_value);
    }

    if (_value)
        m_changeLog.emplace_back(Change::FBRC, _addr, _value);
}

void State::subFBRC(Address const &_addr, u256 const &_value) {
    if (_value == 0)
        return;

    Account *a = account(_addr);
    if (!a || a->FBRC() < _value) {
        // TODO: I expect this never happens.
        cwarn << "_addr: " << _addr << " value " << _value << "  : " << a->FBRC();
        BOOST_THROW_EXCEPTION(NotEnoughCash());
    }


    // Fall back to addBalance().
    addFBRC(_addr, 0 - _value);
}


// FBalance接口实现
u256 State::FBalance(Address const &_id) const {
    if (auto a = account(_id)) {
        return a->FBalance();
    } else {
        return 0;
    }
}


void State::addFBalance(Address const &_addr, u256 const &_value) {
    if (Account *a = account(_addr)) {
        if (!a->isDirty() && a->isEmpty())
            m_changeLog.emplace_back(Change::Touch, _addr);
        a->addFBalance(_value);
    }

    if (_value)
        m_changeLog.emplace_back(Change::FBalance, _addr, _value);
}

void State::subFBalance(Address const &_addr, u256 const &_value) {
    if (_value == 0)
        return;

    Account *a = account(_addr);
    if (!a || a->FBalance() < _value) {
        // TODO: I expect this never happens.
        cwarn << "_addr: " << _addr << " value " << _value << "  : " << a->FBalance();
        BOOST_THROW_EXCEPTION(NotEnoughCash());
    }


    // Fall back to addBalance().
    addFBalance(_addr, 0 - _value);
}


//交易挂单接口
void State::pendingOrder(Address const &_addr, u256 _pendingOrderNum, u256 _pendingOrderPrice,
                         h256 _pendingOrderHash, ex::order_type _pendingOrderType,
                         ex::order_token_type _pendingOrderTokenType,
                         ex::order_buy_type _pendingOrderBuyType, int64_t _nowTime) {
    // add
    //freezeAmount(_addr, _pendingOrderNum, _pendingOrderPrice, _pendingOrderType,
    //           _pendingOrderTokenType, _pendingOrderBuyType);

    u256 _totalSum;
    if (_pendingOrderBuyType == order_buy_type::only_price) {
        _totalSum = _pendingOrderNum * _pendingOrderPrice / PRICEPRECISION;
    } else {
        _totalSum = _pendingOrderPrice;
    }

    std::pair<u256, u256> _pair = {_pendingOrderPrice, _pendingOrderNum};
    order _order = {_pendingOrderHash, _addr, (order_buy_type) _pendingOrderBuyType,
                    (order_token_type) _pendingOrderTokenType, (order_type) _pendingOrderType, _pair, _nowTime};
    std::vector<order> _v = {{_order}};
    std::vector<result_order> _result_v;

    try {
        _result_v = m_exdb.insert_operation(_v, false, true);
    }
    catch (const boost::exception &e) {
        cerror << "this pendingOrder is error :" << diagnostic_information_what(e);
        BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
    }

    u256 CombinationNum = 0;
    u256 CombinationTotalAmount = 0;

    for (uint32_t i = 0; i < _result_v.size(); ++i) {
        result_order _result_order = _result_v[i];

        pendingOrderTransfer(_result_order.sender, _result_order.acceptor, _result_order.amount,
                             _result_order.price, _result_order.type, _result_order.token_type, _result_order.buy_type);

        CombinationNum += _result_order.amount;
        CombinationTotalAmount += _result_order.amount * _result_order.price / PRICEPRECISION;
    }

    if (_pendingOrderBuyType == order_buy_type::only_price) {
        if (_pendingOrderType == order_type::buy && _pendingOrderTokenType == order_token_type::BRC) {
            if (CombinationNum < _pendingOrderNum) {
                subBRC(_addr, _totalSum - CombinationTotalAmount);
                addFBRC(_addr, _totalSum - CombinationTotalAmount);
            }
        } else if (_pendingOrderType == order_type::buy && _pendingOrderTokenType == order_token_type::FUEL) {
            if (CombinationNum < _pendingOrderNum) {
                subBalance(_addr, _totalSum - CombinationTotalAmount);
                addFBalance(_addr, _totalSum - CombinationTotalAmount);
            }

        } else if (_pendingOrderType == order_type::sell && _pendingOrderTokenType == order_token_type::BRC) {
            subBRC(_addr, _pendingOrderNum - CombinationNum);
            addFBRC(_addr, _pendingOrderNum - CombinationNum);
        } else if (_pendingOrderType == order_type::sell && _pendingOrderTokenType == order_token_type::FUEL) {
            subBalance(_addr, _pendingOrderNum - CombinationNum);
            addFBalance(_addr, _pendingOrderNum - CombinationNum);
        }
    }
}

void dev::brc::State::verifyChangeMiner(Address const& _from, EnvInfo const& _envinfo, std::vector<std::shared_ptr<transationTool::operation>> const& _ops){
    // TODO: verify height
    if(config::newChangeHeight() > _envinfo.number()){
        cwarn << "this height:"<< _envinfo.number() << "  can not to changeMiner";
        BOOST_THROW_EXCEPTION(ChangeMinerFailed()<< errinfo_comment("changeMiner height error"));
    }

    if (_ops.empty()){
        cerror << "changeMiner  operation is empty!";
        BOOST_THROW_EXCEPTION(ChangeMinerFailed()<< errinfo_comment("changeMiner operation is empty"));
    }
    std::shared_ptr<transationTool::changeMiner_operation> pen = std::dynamic_pointer_cast<transationTool::changeMiner_operation>(
            _ops[0]);
    if(!pen){
        cerror << "changeMiner  dynamic type field!";
        BOOST_THROW_EXCEPTION(ChangeMinerFailed()<< errinfo_comment("Error Dynamic about changeMiner operation"));
    }
    if(_from != pen->m_before){
        BOOST_THROW_EXCEPTION(ChangeMinerFailed() << errinfo_comment("The originator of the transaction is different from the address of the replacement witness"));
    }
    if(pen->m_after == pen->m_before){
        BOOST_THROW_EXCEPTION(ChangeMinerFailed() << errinfo_comment("replace miner can not same"));
    }

    Account *befor_miner = account(_from);
    if (!befor_miner){
        cerror << "changeMiner  InvalidAddress for beforeAddress!";
        BOOST_THROW_EXCEPTION(ChangeMinerFailed()<< errinfo_comment("not has the Acconut_miner :" + dev::toString(_from)));
    }
    Account *a = account(SysVarlitorAddress);
    Account *a_can = account(SysCanlitorAddress);
    if (!a || !a_can) {
        cerror << "changeMiner  InvalidSysAddress for SysVarlitorAddress!";
        BOOST_THROW_EXCEPTION(ChangeMinerFailed()<< errinfo_comment("can not find sysAddress"));
    }

    auto miner = a->vote_data();
    auto canMiner = a_can->vote_data();

    if (std::find(miner.begin(), miner.end(), _from) == miner.end() &&
            std::find(canMiner.begin(), canMiner.end(), _from) == canMiner.end()){
        cerror << "changeMiner from_address not is miner : "<< _from;
        BOOST_THROW_EXCEPTION(ChangeMinerFailed() <<errinfo_comment(dev::toString(_from)+" not is miner"));
    }
    if (std::find(miner.begin(), miner.end(), pen->m_after) != miner.end() ||
            std::find(canMiner.begin(), canMiner.end(), pen->m_after) != canMiner.end()){
        cerror << "changeMiner after_address already is miner : "<< _from;
        BOOST_THROW_EXCEPTION(ChangeMinerFailed() <<errinfo_comment(dev::toString(_from)+" already is miner"));
    }

    auto mapping_addr = befor_miner->mappingAddress();
    bool is_change = mapping_addr.second == Address() || mapping_addr.second == _from;
    if (!is_change){
        cerror << "changeMiner  can not to change :!"<< pen->m_before;
        BOOST_THROW_EXCEPTION(ChangeMinerFailed() << errinfo_comment("the sender has error mapping_address"));
    }

    Account* a_new = account(pen->m_after);
    if (!a_new){
        createAccount(pen->m_after, {requireAccountStartNonce(), 0, 0});
        a_new = account(pen->m_after);
    }
    if(a_new->mappingAddress().first != Address() && a_new->mappingAddress().second != _from){
        cwarn << " the maping after_addr has alreay mapping_addr :"<< a_new->mappingAddress().first;
        BOOST_THROW_EXCEPTION(ChangeMinerFailed()<< errinfo_comment("the maping after_addr has alreay mapping_addr"));
    }

}

void dev::brc::State::changeMiner(std::vector<std::shared_ptr<transationTool::operation>> const &_ops) {
    if (_ops.empty()){
        cerror << "changeMiner  operation is empty!";
        BOOST_THROW_EXCEPTION(InvalidFunction());
    }
    std::shared_ptr<transationTool::changeMiner_operation> pen = std::dynamic_pointer_cast<transationTool::changeMiner_operation>(
            _ops[0]);
    if(!pen){
        cerror << "changeMiner  dynamic type field!";
        BOOST_THROW_EXCEPTION(InvalidDynamic());
    }

    Account *befor_miner = account(pen->m_before);
    if (!befor_miner){
        cerror << "changeMiner  InvalidAddress for beforeAddress!";
        BOOST_THROW_EXCEPTION(InvalidAddressAddr());
    }
    Account *a = account(SysVarlitorAddress);
    Account *a_can = account(SysCanlitorAddress);
    if (!a || !a_can) {
        cerror << "changeMiner  InvalidSysAddress for SysVarlitorAddress!";
        BOOST_THROW_EXCEPTION(InvalidMinner());
    }
    //a->insertMiner(pen->m_before, pen->m_after, pen->m_blockNumber);

    int ret =0;
    auto miner = a->vote_data();
    auto canMiner = a_can->vote_data();
    if (std::find(miner.begin(), miner.end(), pen->m_before) != miner.end()){
        ret = 1;
    }
    else if (std::find(canMiner.begin(), canMiner.end(), pen->m_before) != canMiner.end()){
        ret = 2;
    }

    if (0==ret){
        cerror << "changeMiner  before not is miner!";
        BOOST_THROW_EXCEPTION(InvalidFunction());
    }

    auto mapping_addr = befor_miner->mappingAddress();
    bool is_change = mapping_addr.second == Address() || mapping_addr.second == pen->m_before;
    if (!is_change){
        cerror << "changeMiner  can not to change :!"<< pen->m_before;
        BOOST_THROW_EXCEPTION(InvalidFunction());
    }

    Account* a_new = account(pen->m_after);
    if (!a_new){
       createAccount(pen->m_after, {requireAccountStartNonce(), 0, 0});
        a_new = account(pen->m_after);
    }
    //change sys data
    Address change_addr = ret == 1 ? SysVarlitorAddress : SysCanlitorAddress;
    Account *change_account = ret == 1 ? a : a_can;
    std::vector<PollData> miners = change_account->vote_data();
    std::vector<PollData> miners_log = miners;
    auto find_ret = std::find(miners.begin(), miners.end(), pen->m_before);
    find_ret->m_addr = pen->m_after;
    change_account->set_vote_data(miners);
    // TODO
    ///rockback log
    //change miner data  A  B  C  D
    /*
     *                      1. A: <0,0>
							2. A -> B :A:<a, b>  B: <a,b>
							3. B -> C :A:<a, c>  B: <0,0>  C:<a,c>
							4. C -> D :A:<a, d>  C: <0,0>  D:<a,d>
							5. D -> A :A:<0, 0>  D: <0,0>
     * */
    Address before_addr = pen->m_before;

    if (mapping_addr.first != Address() && mapping_addr.first != pen->m_before){
        auto  start_a = account(mapping_addr.first);
        if (!start_a){
            createAccount(mapping_addr.first, {requireAccountStartNonce(), 0, 0});
            start_a = account(mapping_addr.first);
        }
        // log
        m_changeLog.emplace_back(Change::NewChangeMiner, mapping_addr.first, start_a->mappingAddress());

        before_addr = mapping_addr.first;
        start_a->setChangeMiner({mapping_addr.first, pen->m_after});

    }
    //log
    m_changeLog.emplace_back(Change::NewChangeMiner, pen->m_before, befor_miner->mappingAddress());
    m_changeLog.emplace_back(Change::NewChangeMiner, pen->m_after, a_new->mappingAddress());
    //data
    a_new->setChangeMiner({before_addr, pen->m_after});
    if(before_addr == pen->m_after ){
        a_new->setChangeMiner({Address(), Address()});
    }

    befor_miner->setChangeMiner({before_addr, pen->m_after});
    if (befor_miner->mappingAddress().second != pen->m_before && befor_miner->mappingAddress().first != pen->m_before){
        befor_miner->setChangeMiner({Address(), Address()});
    }
    m_changeLog.emplace_back(Change::MinnerSnapshot, change_addr, miners_log);

}

Account *dev::brc::State::getSysAccount() {
    return account(SysVarlitorAddress);
}

//void dev::brc::State::pendingOrders(Address const &_addr, int64_t _nowTime, h256 _pendingOrderHash,
std::pair<u256 ,u256> dev::brc::State::pendingOrders(Address const &_addr, int64_t _nowTime, int64_t blockHeight, h256 _pendingOrderHash,
                                    std::vector<std::shared_ptr<transationTool::operation>> const &_ops) {
    std::vector<ex_order> _v;
    std::vector<result_order> _result_v;
    std::set<order_type> _set;
    bigint total_free_brc = 0;
    bigint total_free_balance = 0;
    std::pair<u256, u256> _exNumPair;
    u256 _exCookieNum = 0;
    u256 _exBRCNum = 0;
    ExdbState _exdbState(*this);
    for (auto const &val : _ops) {
        std::shared_ptr<transationTool::pendingorder_opearaion> pen = std::dynamic_pointer_cast<transationTool::pendingorder_opearaion>(
                val);
        if (!pen) {
            cerror << "pendingOrders  dynamic type field!";
            BOOST_THROW_EXCEPTION(InvalidDynamic());
        }

        u256 _price = 0;
        if (pen->m_Pendingorder_buy_type == order_buy_type::all_price && pen->m_Pendingorder_type == order_type::buy) {
            _price = pen->m_Pendingorder_price * PRICEPRECISION;
        } else {
            _price = pen->m_Pendingorder_price;
        }

        ex_order _order = {_pendingOrderHash, _addr, _price, pen->m_Pendingorder_num, pen->m_Pendingorder_num, _nowTime,
                           (order_type) pen->m_Pendingorder_type, (order_token_type) pen->m_Pendingorder_Token_type,
                           (order_buy_type) pen->m_Pendingorder_buy_type};
        _v.push_back(_order);

        if (pen->m_Pendingorder_buy_type == order_buy_type::only_price) {
            if (pen->m_Pendingorder_type == order_type::buy && pen->m_Pendingorder_Token_type == order_token_type::FUEL)
                total_free_brc += pen->m_Pendingorder_num * pen->m_Pendingorder_price / PRICEPRECISION;
            else if (pen->m_Pendingorder_type == order_type::sell &&
                     pen->m_Pendingorder_Token_type == order_token_type::FUEL)
                total_free_balance += pen->m_Pendingorder_num;
        }
    }
    for (auto _val : _v) {
        try {
            //default.
            if(config::changeExchange() >= blockHeight){
                _result_v = _exdbState.insert_operation(_val);
            }
            else {
                //TODO use new db.
            }
        }
        catch (const boost::exception &e) {
            cerror << "this pendingOrder is error :" << diagnostic_information_what(e);
            BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
        }

        for (uint32_t i = 0; i < _result_v.size(); ++i) {
            result_order _result_order = _result_v[i];

            pendingOrderTransfer(_result_order.sender, _result_order.acceptor, _result_order.amount,
                                 _result_order.price, _result_order.type, _result_order.token_type,
                                 _result_order.buy_type);
            if(_result_order.type == order_type::buy && _result_order.token_type == order_token_type::FUEL && _result_order.buy_type == order_buy_type::all_price)
            {
                _exCookieNum += _result_order.amount - _result_order.amount / MATCHINGFEERATIO;
                _exBRCNum += _result_order.amount * _result_order.price / PRICEPRECISION;
            }
            if (_result_order.buy_type == order_buy_type::only_price) {
                if (_result_order.type == order_type::buy && _result_order.token_type == order_token_type::FUEL)
                    total_free_brc -= _result_order.amount * _result_order.old_price / PRICEPRECISION;
                else if (_result_order.type == order_type::sell && _result_order.token_type == order_token_type::FUEL)
                    total_free_balance -= _result_order.amount;
            }
            if (_result_order.acceptor == systemAddress) {
                _set.emplace(_result_order.type);
            } else {
                continue;
            }

        }
    }

    if (total_free_balance < 0 || total_free_brc < 0) {
        cerror << " this pindingOrder's free_balance or free_brc is error... balance:" << total_free_balance << " brc:"
               << total_free_brc;
        BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
    }
    if (total_free_balance > 0) {
        subBalance(_addr, (u256) total_free_balance);
        addFBalance(_addr, (u256) total_free_balance);
    }
    if (total_free_brc) {
        subBRC(_addr, (u256) total_free_brc);
        addFBRC(_addr, (u256) total_free_brc);
    }

    if (_set.size() > 0) {
        systemAutoPendingOrder(_set, _nowTime);
    }

    _exNumPair = std::make_pair(_exCookieNum, _exBRCNum);
    return _exNumPair;
}



void State::systemAutoPendingOrder(std::set<order_type> const &_set, int64_t _nowTime) {
    std::vector<result_order> _result_v;
    std::set<order_type> _autoSet;
    std::vector<ex_order> _v;
    u256 _needBrc = 0;
    u256 _needCookie = 0;
    ExdbState _exdbState(*this);
    for (auto it : _set) {
        if (it == order_type::buy) {
            if (BRC(systemAddress) < u256(std::string("10000000000000"))) {
                return;
            }
            u256 _num = BRC(systemAddress) * PRICEPRECISION / BUYCOOKIE / 10000 * 10000;
            _needBrc = _num * u256(BUYCOOKIE) / PRICEPRECISION;
            RLPStream _rlp(3);
            _rlp << _nowTime << account(ExdbSystemAddress)->getExOrder().size() << _num;
            h256 _trHash = dev::sha3(_rlp.out());
            ex_order _order = {_trHash, systemAddress, u256(BUYCOOKIE), _num, _num, _nowTime, order_type::buy,
                               order_token_type::FUEL, order_buy_type::only_price};
            _v.push_back(_order);
        } else if (it == order_type::sell) {
            if (balance(systemAddress) < u256(std::string("10000000000000"))) {
                return;
            }
            u256 _num = balance(systemAddress);
            _needCookie = _num;

            RLPStream _rlp(3);
            _rlp << _nowTime << account(ExdbSystemAddress)->getExOrder().size() << _num;
            h256 _trHash = dev::sha3(_rlp.out());

            std::pair<u256, u256> _pair = {u256(SELLCOOKIE), _num};
            ex_order _order = {_trHash, systemAddress, u256(SELLCOOKIE), _num, _num, _nowTime, order_type::sell,
                               order_token_type::FUEL, order_buy_type::only_price};
            _v.push_back(_order);
        }
    }

    for (auto _val : _v) {
        try {
            _result_v = _exdbState.insert_operation(_val);
        }
        catch (const boost::exception &e) {
            cerror << "this pendingOrder is error :" << diagnostic_information_what(e);
            BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
        }

        for (auto _result : _result_v) {
            pendingOrderTransfer(_result.sender, _result.acceptor, _result.amount,
                                 _result.price, _result.type, _result.token_type, _result.buy_type);

            if (_result.buy_type == order_buy_type::only_price) {
                if (_result.type == order_type::buy && _result.token_type == order_token_type::FUEL)
                    _needBrc -= _result.amount * _result.old_price / PRICEPRECISION;
                else if (_result.type == order_type::sell && _result.token_type == order_token_type::FUEL)
                    _needCookie -= _result.amount;
            }
            _autoSet.emplace(_result.type);
        }
    }

    if (_needBrc < 0 || _needCookie < 0) {
        cerror << " this pindingOrder's free_balance or free_brc is error... balance:" << _needCookie << " brc:"
               << _needBrc;
        BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
    }
    if (_needCookie > 0) {
        subBalance(systemAddress, _needCookie);
        addFBalance(systemAddress, _needCookie);
    }
    if (_needBrc) {
        subBRC(systemAddress, _needBrc);
        addFBRC(systemAddress, _needBrc);
    }
    if (_autoSet.size() > 0) {
        systemAutoPendingOrder(_autoSet, _nowTime);
    }
}

void State::pendingOrderTransfer(Address const &_from, Address const &_to, u256 _toPendingOrderNum,
                                 u256 _toPendingOrderPrice, ex::order_type _pendingOrderType,
                                 ex::order_token_type _pendingOrderTokenType,
                                 ex::order_buy_type _pendingOrderBuyTypes) {

    u256 _fromFee = 0;//(_toPendingOrderNum * _toPendingOrderPrice / PRICEPRECISION) / MATCHINGFEERATIO;
    u256 _toFee = 0;//_toPendingOrderNum / MATCHINGFEERATIO;

    if (_pendingOrderType == order_type::buy &&
        _pendingOrderTokenType == order_token_type::FUEL &&
        (_pendingOrderBuyTypes == order_buy_type::only_price || _pendingOrderBuyTypes == order_buy_type::all_price)) {
        if (_from != dev::systemAddress) {
            _fromFee = _toPendingOrderNum / MATCHINGFEERATIO;
        }
        if (_to != dev::systemAddress) {
            _toFee = (_toPendingOrderNum * _toPendingOrderPrice / PRICEPRECISION) / MATCHINGFEERATIO;
        }
        subBRC(_from, _toPendingOrderPrice * _toPendingOrderNum / PRICEPRECISION);
        addBRC(_to, _toPendingOrderPrice * _toPendingOrderNum / PRICEPRECISION - _toFee);
        subFBalance(_to, _toPendingOrderNum);
        addBalance(_from, _toPendingOrderNum - _fromFee);
        addBRC(dev::PdSystemAddress, _toFee);
        addBalance(dev::PdSystemAddress, _fromFee);
    } else if (_pendingOrderType == order_type::sell &&
               _pendingOrderTokenType == order_token_type::FUEL &&
               (_pendingOrderBuyTypes == order_buy_type::only_price ||
                _pendingOrderBuyTypes == order_buy_type::all_price)) {
        if (_from != dev::systemAddress) {
            _fromFee = (_toPendingOrderNum * _toPendingOrderPrice / PRICEPRECISION) / MATCHINGFEERATIO;
        }
        if (_to != dev::systemAddress) {
            _toFee = _toPendingOrderNum / MATCHINGFEERATIO;
        }
        subBalance(_from, _toPendingOrderNum);
        addBalance(_to, _toPendingOrderNum - _toFee);
        subFBRC(_to, _toPendingOrderNum * _toPendingOrderPrice / PRICEPRECISION);
        addBRC(_from, _toPendingOrderNum * _toPendingOrderPrice / PRICEPRECISION - _fromFee);
        addBRC(dev::PdSystemAddress, _fromFee);
        addBalance(dev::PdSystemAddress, _toFee);
    }
}

void State::freezeAmount(Address const &_addr, u256 _pendingOrderNum, u256 _pendingOrderPrice,
                         ex::order_type _pendingOrderType, ex::order_token_type _pendingOrderTokenType,
                         ex::order_buy_type _pendingOrderBuyType) {
    if (_pendingOrderType == order_type::buy && _pendingOrderTokenType == order_token_type::BRC &&
        _pendingOrderBuyType == order_buy_type::only_price) {
        subBRC(_addr, _pendingOrderNum * _pendingOrderPrice);
        addFBRC(_addr, _pendingOrderNum * _pendingOrderPrice);
    } else if (_pendingOrderType == order_type::buy &&
               _pendingOrderTokenType == order_token_type::BRC &&
               _pendingOrderBuyType == order_buy_type::all_price) {
        subBRC(_addr, _pendingOrderPrice);
        addFBRC(_addr, _pendingOrderPrice);
    } else if (_pendingOrderType == order_type::buy &&
               _pendingOrderTokenType == order_token_type::FUEL &&
               _pendingOrderBuyType == order_buy_type::only_price) {
        subBalance(_addr, _pendingOrderNum * _pendingOrderPrice);
        addFBalance(_addr, _pendingOrderNum * _pendingOrderPrice);
    } else if (_pendingOrderType == order_type::buy &&
               _pendingOrderTokenType == order_token_type::FUEL &&
               _pendingOrderBuyType == order_buy_type::all_price) {
        subBalance(_addr, _pendingOrderPrice);
        addFBalance(_addr, _pendingOrderPrice);
    } else if (_pendingOrderType == order_type::sell &&
               _pendingOrderTokenType == order_token_type::BRC &&
               (_pendingOrderBuyType == order_buy_type::only_price ||
                _pendingOrderBuyType == order_buy_type::all_price)) {
        subBRC(_addr, _pendingOrderNum);
        addFBRC(_addr, _pendingOrderNum);
    } else if (_pendingOrderType == order_type::sell &&
               _pendingOrderTokenType == order_token_type::FUEL &&
               (_pendingOrderBuyType == order_buy_type::only_price ||
                _pendingOrderBuyType == order_buy_type::all_price)) {
        subBalance(_addr, _pendingOrderNum);
        addFBalance(_addr, _pendingOrderNum);
    }
}

Json::Value State::queryExchangeReward(Address const &_address, unsigned _blockNum) {
    trySnapshotWithMinerMapping(_address, _blockNum);
    std::pair<u256, u256> _pair ={0,0};

    if (_blockNum < config::newChangeHeight()){
        _pair = anytime_receivingPdFeeIncome(_address, (int64_t) _blockNum, false);
    }
    /// new changeMiner fork
    if(_blockNum >= config::newChangeHeight()){
        do{
            auto miner_mapping = minerMapping(_address);
            if (miner_mapping.first != Address() && miner_mapping.first == _address &&
                miner_mapping.second != _address) {
                cwarn << "the minner address is changed by other:" << miner_mapping.second;
                break;
            }
            _pair = anytime_receivingPdFeeIncome(_address, (int64_t) _blockNum, false);
            if (miner_mapping.first != Address() && miner_mapping.first != _address) {
                auto ret = anytime_receivingPdFeeIncome(miner_mapping.first, (int64_t) _blockNum, false);
                _pair.first += ret.first;
                _pair.second += ret.second;
            }
        }
        while (false);
    }

    Json::Value res;
    Json::Value ret;
    ret["BRC"] = toJS(_pair.first);
    ret["Cookies"] = toJS(_pair.second);
    res["queryExchangeReward"] = ret;
    return res;
}

Json::Value State::queryBlcokReward(Address const &_address, unsigned _blockNum) {
    trySnapshotWithMinerMapping(_address, _blockNum);
    u256 _cookieFee =0;

    if (_blockNum < config::newChangeHeight()){
        _cookieFee = rpcqueryBlcokReward(_address, _blockNum);
    }
    /// new changeMiner fork
    if(_blockNum >= config::newChangeHeight()){
        do{
            auto miner_mapping = minerMapping(_address);
            if (miner_mapping.first != Address() && miner_mapping.first == _address &&
                miner_mapping.second != _address) {
                cwarn << "the minner address is changed by other:" << miner_mapping.second;
                break;
            }
            _cookieFee = rpcqueryBlcokReward(_address, _blockNum);
            if (miner_mapping.first != Address() && miner_mapping.first != _address && miner_mapping.second == _address) {
                auto ret = rpcqueryBlcokReward(miner_mapping.first, (int64_t) _blockNum);
                if (ret >0)
                    _cookieFee += ret;
            }
        }
        while (false);
    }

    Json::Value _retVal;
    _retVal["CookieFeeIncome"] = toJS(_cookieFee);
    return _retVal;

}

u256 State::rpcqueryBlcokReward(Address const &_address, unsigned _blockNum){
    auto a = account(_address);
    ReceivedCookies _receivedCookies = a->get_received_cookies();
    ReceivedCookies _oldreceivedCookies = a->get_received_cookies();
    VoteSnapshot _votesnapshot = a->vote_snashot();
    std::pair<u256, Votingstage> _pair = config::getVotingCycle(_blockNum);
    u256 _numberofrounds = config::getvoteRound(_receivedCookies.m_numberofRound);
    u256 _cookieFee = 0;
    u256 _isMainNodeFee = 0;
    u256 _voteNodeFee = 0;
    std::map<u256, std::map<Address, u256>>::const_iterator _voteDataIt = _votesnapshot.m_voteDataHistory.find(
            _numberofrounds - 1);

    // If you receive the account, you will receive the income from the block node account.

    //Receive an account to receive voting dividends
    for (; _voteDataIt != _votesnapshot.m_voteDataHistory.end(); _voteDataIt++) {
        std::map<Address, std::pair<u256, u256>> _totalMap;
        std::map<u256, u256>::const_iterator _pollDataIt = _votesnapshot.m_pollNumHistory.find(_voteDataIt->first);
        auto _pollNum = _pollDataIt->second;
        if (_pollNum > 0) {
            u256 _pollFee = 0;
            if (_pollDataIt->first + 1 < _pair.first) {
                if (_votesnapshot.m_blockSummaryHistory.count(_pollDataIt->first + 1)) {
                    _pollFee = _votesnapshot.m_blockSummaryHistory.find(_pollDataIt->first + 1)->second;
                }
            } else {
                _pollFee = a->CookieIncome();
            }
            _isMainNodeFee = _pollFee - (_pollFee / 2 / _pollNum * _pollNum);
            CFEE_LOG << "_pollFee:" << _pollFee;
            CFEE_LOG << "_isMainNodeFee:" << _isMainNodeFee;
            if (!_totalMap.count(_address)) {
                _totalMap[_address] = std::pair<u256, u256>(_pollFee, _isMainNodeFee);
            } else {
                std::pair<u256, u256> _pair = _totalMap[_address];
                _pair.second += _isMainNodeFee;
                _totalMap[_address] = _pair;
            }
        }
        for (auto _voteIt : _voteDataIt->second) {
            auto _pollAddr = account(_voteIt.first);
            trySnapshotWithMinerMapping(_voteIt.first, _blockNum);
            VoteSnapshot _pollVoteSnapshot = _pollAddr->vote_snashot();
            auto _pollMap = _pollVoteSnapshot.m_pollNumHistory;
            u256 _pollNum = _pollMap.find(_voteDataIt->first)->second;
            u256 _pollCookieFee = 0;
            u256 _receivedNum = 0;
            u256 _pollFeeIncome = 0;
            u256 _numTotalFee = 0;
            if (_pollNum > 0) {
                if (_voteDataIt->first + 1 < _pair.first) {
                    if (_pollVoteSnapshot.m_blockSummaryHistory.count(_voteDataIt->first + 1)) {
                        _pollCookieFee = _pollVoteSnapshot.m_blockSummaryHistory.find(_voteDataIt->first + 1)->second;
                    }
                } else {
                    _pollCookieFee = _pollAddr->CookieIncome();
                }
                _pollFeeIncome = _pollCookieFee / 2 / _pollNum * _voteIt.second;
                if (!_totalMap.count(_voteIt.first)) {
                    _totalMap[_voteIt.first] = std::pair<u256, u256>(_pollFeeIncome, _pollFeeIncome);
                } else {
                    std::pair<u256, u256> _pair = _totalMap[_voteIt.first];
                    _pair.second += _pollFeeIncome;
                    _totalMap[_voteIt.first] = _pair;
                }
            }
        }
        for (auto _totalIt : _totalMap) {
            u256 _receivedNum = 0;
            if (_receivedCookies.m_received_cookies.count(_voteDataIt->first + 1)) {
                std::map<Address, std::pair<u256, u256>> _addrReceivedCookie = _receivedCookies.m_received_cookies[
                        _voteDataIt->first + 1];
                if (_addrReceivedCookie.count(_totalIt.first)) {
                    _receivedNum += _addrReceivedCookie.find(_totalIt.first)->second.second;
                }
            }
            _cookieFee += _totalIt.second.second - _receivedNum;
            a->addSetreceivedCookie(_voteDataIt->first + 1, _totalIt.first,
                                    std::pair<u256, u256>(_totalIt.second.first, _totalIt.second.second));
        }
    }
    return _cookieFee;
}

Json::Value State::pendingOrderPoolMsg(uint8_t _order_type, uint8_t _order_token_type, u256 _gsize) {
    ExdbState _exdbState(*this);
    uint32_t _maxSize = (uint32_t) _gsize;
    _maxSize = _maxSize >= LIMITE_NUMBER ? LIMITE_NUMBER : _maxSize;
    std::vector<exchange_order> _v = _exdbState.get_order_by_type(
            (order_type) _order_type, (order_token_type) _order_token_type, (uint32_t) _maxSize);

    Json::Value _JsArray;
    for (auto val : _v) {
        Json::Value _value;
        _value["Address"] = toJS(val.sender);
        _value["Hash"] = toJS(val.trxid);
        _value["price"] = std::string(val.price);
        _value["token_amount"] = std::string(val.token_amount);
        _value["source_amount"] = std::string(val.source_amount);
        _value["create_time"] = toJS(val.create_time);
        std::tuple<std::string, std::string, std::string> _resultTuple = enumToString(val.type, val.token_type,
                                                                                      (ex::order_buy_type) 0);
        _value["order_type"] = get<0>(_resultTuple);
        _value["order_token_type"] = get<1>(_resultTuple);
        _JsArray.append(_value);
    }

    return _JsArray;
}

Json::Value State::pendingOrderPoolForAddrMsg(Address _a, uint32_t _maxSize) {
    ExdbState _exdbState(*this);
    _maxSize = _maxSize >= LIMITE_NUMBER ? LIMITE_NUMBER : _maxSize;
    std::vector<exchange_order> _v = _exdbState.get_order_by_address(_a, _maxSize);
    Json::Value _JsArray;

    for (auto val : _v) {
        Json::Value _value;
        _value["Address"] = toJS(val.sender);
        _value["Hash"] = toJS(val.trxid);
        _value["price"] = std::string(val.price);
        _value["token_amount"] = std::string(val.token_amount);
        _value["source_amount"] = std::string(val.source_amount);
        _value["create_time"] = toJS(val.create_time);
        std::tuple<std::string, std::string, std::string> _resultTuple = enumToString(val.type, val.token_type,
                                                                                      (ex::order_buy_type) 0);
        _value["order_type"] = get<0>(_resultTuple);
        _value["order_token_type"] = get<1>(_resultTuple);
        _JsArray.append(_value);
    }

    return _JsArray;
}

Json::Value State::successPendingOrderMsg(uint32_t _maxSize) {
    ExdbState _exdbState(*this);
    _maxSize = _maxSize >= LIMITE_NUMBER ? LIMITE_NUMBER : _maxSize;
    std::vector<result_order> _v = _exdbState.get_result_orders_by_news(_maxSize);
    Json::Value _JsArray;

    for (auto val : _v) {
        Json::Value _value;
        _value["Address"] = toJS(val.sender);
        _value["Acceptor"] = toJS(val.acceptor);
        _value["Hash"] = toJS(val.send_trxid);
        _value["AcceptorHash"] = toJS(val.to_trxid);
        _value["price"] = std::string(val.price);
        _value["amount"] = std::string(val.amount);
        _value["create_time"] = toJS(val.create_time);
        std::tuple<std::string, std::string, std::string> _resultTuple = enumToString(val.type, val.token_type,
                                                                                      val.buy_type);
        _value["order_type"] = get<0>(_resultTuple);
        _value["order_token_type"] = get<1>(_resultTuple);
        _value["order_buy_type"] = get<2>(_resultTuple);
        _JsArray.append(_value);
    }

    return _JsArray;
}

Json::Value State::successPendingOrderForAddrMsg(dev::Address _a, int64_t _minTime, int64_t _maxTime,
                                                 uint32_t _maxSize) {
    ExdbState _exdbState(*this);
    _maxSize = _maxSize >= LIMITE_NUMBER ? LIMITE_NUMBER : _maxSize;
    std::vector<result_order> _retV = _exdbState.get_result_orders_by_address(_a, _minTime, _maxTime, _maxSize);
    Json::Value _JsArray;

    for (auto val : _retV) {
        Json::Value _value;
        _value["Address"] = toJS(val.sender);
        _value["Acceptor"] = toJS(val.acceptor);
        _value["Hash"] = toJS(val.send_trxid);
        _value["AcceptorHash"] = toJS(val.to_trxid);
        _value["price"] = std::string(val.price);
        _value["amount"] = std::string(val.amount);
        _value["create_time"] = toJS(val.create_time);
        std::tuple<std::string, std::string, std::string> _resultTuple = enumToString(val.type, val.token_type,
                                                                                      val.buy_type);
        _value["order_type"] = get<0>(_resultTuple);
        _value["order_token_type"] = get<1>(_resultTuple);
        _value["order_buy_type"] = get<2>(_resultTuple);
        _JsArray.append(_value);
    }

    return _JsArray;

}

std::tuple<std::string, std::string, std::string>
State::enumToString(ex::order_type type, ex::order_token_type token_type, ex::order_buy_type buy_type) {
    std::string _type, _token_type, _buy_type;
    switch (type) {
        case dev::brc::ex::order_type::sell:
            _type = std::string("sell");
            break;
        case dev::brc::ex::order_type::buy:
            _type = std::string("buy");
            break;
        default:
            _type = std::string("NULL");
            break;
    }

    switch (token_type) {
        case dev::brc::ex::order_token_type::BRC:
            _token_type = std::string("BRC");
            break;
        case dev::brc::ex::order_token_type::FUEL:
            _token_type = std::string("FUEL");
            break;
        default:
            _token_type = std::string("NULL");
            break;
    }

    switch (buy_type) {
        case dev::brc::ex::order_buy_type::all_price:
            _buy_type = std::string("all_price");
            break;
        case dev::brc::ex::order_buy_type::only_price:
            _buy_type = std::string("only_price");
            break;
        default:
            _buy_type = std::string("NULL");
            break;
    }

    std::tuple<std::string, std::string, std::string> _result = std::make_tuple(_type, _token_type, _buy_type);
    return _result;
}


void State::cancelPendingOrder(h256 _pendingOrderHash) {
    ctrace << "cancle pendingorder";
    std::vector<ex::order> _resultV;
    try {
        std::vector<h256> _hashV = {_pendingOrderHash};
        _resultV = m_exdb.cancel_order_by_trxid(_hashV, false);
    }
    catch (const boost::exception &e) {
        cwarn << "cancelPendingorder Error :" << _pendingOrderHash;
    }

    for (auto val : _resultV) {
        if (val.type == order_type::buy && val.token_type == order_token_type::FUEL) {
            subFBalance(val.sender, val.price_token.second * val.price_token.first);
            addBalance(val.sender, val.price_token.second * val.price_token.first);
        } else if (val.type == order_type::sell && val.token_type == order_token_type::FUEL) {
            subFBalance(val.sender, val.price_token.second);
            addBalance(val.sender, val.price_token.second);
        }
    }
}

void dev::brc::State::cancelPendingOrders(std::vector<std::shared_ptr<transationTool::operation>> const &_ops, int64_t _blockHeight) {
    ctrace << "cancle pendingorder";
    ex::order val;
    std::vector<h256> _hashV;
    ExdbState _exdbState(*this);
    for (auto const &val : _ops) {
        std::shared_ptr<transationTool::cancelPendingorder_operation> can_pen = std::dynamic_pointer_cast<transationTool::cancelPendingorder_operation>(
                val);
        if (!can_pen) {
            cerror << "cancelPendingOrders  dynamic type field!";
            BOOST_THROW_EXCEPTION(InvalidDynamic());
        }
        _hashV.push_back({can_pen->m_hash});
    }
    for (auto _val : _hashV) {
        try {
            if(_blockHeight <= config::changeExchange())
                val = _exdbState.cancel_order_by_trxid(_val);
            else{
                //TODO new ex
            }
        }
        catch (Exception &e) {
            cwarn << "cancelPendingorder Error :" << e.what();
            BOOST_THROW_EXCEPTION(CancelPendingOrderFiled());
        }

        if (val.type == order_type::buy && val.token_type == order_token_type::FUEL) {
            subFBRC(val.sender, val.price_token.second * val.price_token.first / PRICEPRECISION);
            addBRC(val.sender, val.price_token.second * val.price_token.first / PRICEPRECISION);
        } else if (val.type == order_type::sell && val.token_type == order_token_type::FUEL) {
            subFBalance(val.sender, val.price_token.second);
            addBalance(val.sender, val.price_token.second);
        }
    }
}

void State::addBlockReward(Address const &_addr, u256 _blockNum, u256 _rewardNum) {
    std::pair<u256, u256> _pair = {_blockNum, _rewardNum};
    if (auto a = account(_addr)) {
        a->addBlockRewardRecoding(_pair);
    } else {
        createAccount(_addr, {requireAccountStartNonce(), 0});
        auto _a = account(_addr);
        _a->addBlockRewardRecoding(_pair);
    }

    if (_rewardNum) {
        m_changeLog.emplace_back(_addr, std::make_pair(_blockNum, _rewardNum));
    }
}


void State::addCooikeIncomeNum(const dev::Address &_addr, const dev::u256 &_value) {
    if (auto a = account(_addr)) {
        a->addCooikeIncome(_value);
    } else {
        BOOST_THROW_EXCEPTION(InvalidAddressAddr());
    }

    if (_value) {
        m_changeLog.emplace_back(Change::CooikeIncomeNum, _addr, _value);
    }
}

void State::subCookieIncomeNum(const dev::Address &_addr, const dev::u256 &_value) {
    if (_value == 0) {
        return;
    }

    auto a = account(_addr);
    if (!a || a->CookieIncome()) {
        BOOST_THROW_EXCEPTION(NotEnoughCash() << errinfo_comment(
                std::string("Account does not exist or account CookieIncome is too low ")));
    }

    addCooikeIncomeNum(_addr, 0 - _value);
}

void State::setCookieIncomeNum(const dev::Address &_addr, const dev::u256 &_value) {
    Account *a = account(_addr);
    u256 original = a ? a->CookieIncome() : 0;

    // Fall back to addBalance().
    addCooikeIncomeNum(_addr, _value - original);
}

std::unordered_map<Address, u256> State::incomeSummary(const dev::Address &_addr, uint32_t _snapshotNum) {
    if (auto a = account(_addr)) {
        return a->findSnapshotSummary(_snapshotNum);
    } else {
        return std::unordered_map<Address, u256>();
    }
}

void
State::receivingIncome(const dev::Address &_addr, std::vector<std::shared_ptr<transationTool::operation>> const &_ops,
                       int64_t _blockNum) {
    trySnapshotWithMinerMapping(_addr, _blockNum);
    for (auto _it : _ops) {
        std::shared_ptr<transationTool::receivingincome_operation> _op = std::dynamic_pointer_cast<transationTool::receivingincome_operation>(
                _it);
        if (!_op) {
            BOOST_THROW_EXCEPTION(
                    receivingincomeFiled() << errinfo_comment(std::string("receivingincome_operation is error")));
        }

        ReceivingType _receType = (ReceivingType) _op->m_receivingType;
        // TODO frok code
        if(_blockNum < config::newChangeHeight()) {
            if (_receType == ReceivingType::RBlockFeeIncome) {
                receivingBlockFeeIncome(_addr, _blockNum);
            } else if (_receType == ReceivingType::RPdFeeIncome) {
                anytime_receivingPdFeeIncome(_addr, _blockNum);
            }
        }
        else{
            // new change fork
            auto miner_mapping = minerMapping(_addr);
            Address mapping_addr = miner_mapping.first == Address() ? _addr : miner_mapping.first;
            if (_receType == ReceivingType::RBlockFeeIncome) {
                receivingBlockFeeIncome(_addr, _blockNum);
                if(mapping_addr != _addr){
                    auto ret = receivingBlockFeeIncome(mapping_addr, _blockNum);
                    if (ret >0){
                        addBalance(_addr, ret);
                        subBalance(mapping_addr, ret);
                    }
                }
            } else if (_receType == ReceivingType::RPdFeeIncome) {
                anytime_receivingPdFeeIncome(_addr, _blockNum);
                if (mapping_addr != _addr){
                    auto pair = anytime_receivingPdFeeIncome(mapping_addr, _blockNum, true);
                    if(pair.first > 0 || pair.second > 0){
                        addBalance(_addr, pair.second);
                        addBRC(_addr, pair.first);
                        subBalance(mapping_addr, pair.second);
                        subBRC(mapping_addr, pair.first);
                    }
                }
            }
        }
    }
}

u256 State::receivingBlockFeeIncome(const dev::Address &_addr, int64_t _blockNum) {
    auto a = account(_addr);
    ReceivedCookies _receivedCookies = a->get_received_cookies();
    ReceivedCookies _oldreceivedCookies = a->get_received_cookies();
    VoteSnapshot _votesnapshot = a->vote_snashot();
    std::pair<u256, Votingstage> _pair = config::getVotingCycle(_blockNum);
    u256 _numberofrounds = config::getvoteRound(_receivedCookies.m_numberofRound);
    u256 _cookieFee = 0;
    u256 _isMainNodeFee = 0;
    u256 _voteNodeFee = 0;
    std::map<u256, std::map<Address, u256>>::const_iterator _voteDataIt = _votesnapshot.m_voteDataHistory.find(
            _numberofrounds - 1);
    //CFEE_LOG << "BEFEOR :" << _receivedCookies;

    // If you receive the account, you will receive the income from the block node account.

    //Receive an account to receive voting dividends
    for (; _voteDataIt != _votesnapshot.m_voteDataHistory.end(); _voteDataIt++) {
        std::map<Address, std::pair<u256, u256>> _totalMap;
        std::map<u256, u256>::const_iterator _pollDataIt = _votesnapshot.m_pollNumHistory.find(_voteDataIt->first);
        auto _pollNum = _pollDataIt->second;
        if (_pollNum > 0) {
            u256 _pollFee = 0;
            if (_pollDataIt->first + 1 < _pair.first) {
                if (_votesnapshot.m_blockSummaryHistory.count(_pollDataIt->first + 1)) {
                    _pollFee = _votesnapshot.m_blockSummaryHistory.find(_pollDataIt->first + 1)->second;
                }
            } else {
                _pollFee = a->CookieIncome();
            }
            _isMainNodeFee = _pollFee - (_pollFee / 2 / _pollNum * _pollNum);
            if (!_totalMap.count(_addr)) {
                _totalMap[_addr] = std::pair<u256, u256>(_pollFee, _isMainNodeFee);
            } else {
                std::pair<u256, u256> _pair = _totalMap[_addr];
                _pair.second += _isMainNodeFee;
                _totalMap[_addr] = _pair;
            }
        }
        for (auto _voteIt : _voteDataIt->second) {
            auto _pollAddr = account(_voteIt.first);
            trySnapshotWithMinerMapping(_voteIt.first, _blockNum);
            VoteSnapshot _pollVoteSnapshot = _pollAddr->vote_snashot();
            auto _pollMap = _pollVoteSnapshot.m_pollNumHistory;
            u256 _pollNum = _pollMap.find(_voteDataIt->first)->second;
            u256 _pollCookieFee = 0;
            u256 _receivedNum = 0;
            u256 _pollFeeIncome = 0;
            u256 _numTotalFee = 0;
            if (_pollNum > 0) {
                if (_voteDataIt->first + 1 < _pair.first) {
                    if (_pollVoteSnapshot.m_blockSummaryHistory.count(_voteDataIt->first + 1)) {
                        _pollCookieFee = _pollVoteSnapshot.m_blockSummaryHistory.find(_voteDataIt->first + 1)->second;
                    }
                } else {
                    _pollCookieFee = _pollAddr->CookieIncome();
                }
                _pollFeeIncome = _pollCookieFee / 2 / _pollNum * _voteIt.second;
                if (!_totalMap.count(_voteIt.first)) {
                    _totalMap[_voteIt.first] = std::pair<u256, u256>(_pollFeeIncome, _pollFeeIncome);
                } else {
                    std::pair<u256, u256> _pair = _totalMap[_voteIt.first];
                    _pair.second += _pollFeeIncome;
                    _totalMap[_voteIt.first] = _pair;
                }
            }
        }
        for (auto _totalIt : _totalMap) {
            u256 _receivedNum = 0;
            if (_receivedCookies.m_received_cookies.count(_voteDataIt->first + 1)) {
                std::map<Address, std::pair<u256, u256>> _addrReceivedCookie = _receivedCookies.m_received_cookies[
                        _voteDataIt->first + 1];
                if (_addrReceivedCookie.count(_totalIt.first)) {
                    _receivedNum += _addrReceivedCookie.find(_totalIt.first)->second.second;
                }
            }
            _cookieFee += _totalIt.second.second - _receivedNum;
            a->addSetreceivedCookie(_voteDataIt->first + 1, _totalIt.first,
                                    std::pair<u256, u256>(_totalIt.second.first, _totalIt.second.second));
        }
    }
    //CFEE_LOG << "_cookieFee" << _cookieFee;
    //CFEE_LOG << "now" << a->get_received_cookies();
    addBalance(_addr, _cookieFee);
    a->updateNumofround(_pair.first);
    m_changeLog.emplace_back(Change::ReceiveCookies, _addr, _oldreceivedCookies);
    return _cookieFee;
}

//void State::receivingBlockFeeIncome(const dev::Address &_addr, int64_t _blockNum)
//{
//    u256 _income = 0;
//    auto a = account(_addr);
//    VoteSnapshot _voteSnapshot = a->vote_snashot();
//    u256 _numberofrounds = _voteSnapshot.numberofrounds;
//
//    std::pair<uint32_t, Votingstage> _pair = config::getVotingCycle(_blockNum);
//    u256 rounds = _pair.first > 0 ? _pair.first - 1 : 0;
//
//    std::map<u256, std::map<Address, u256>>::iterator _voteDataIt = _voteSnapshot.m_voteDataHistory.find(0 + 1);
//    std::map<u256, u256>::iterator _pollDataIt = _voteSnapshot.m_pollNumHistory.find(_numberofrounds + 1);      //first->rounds second->polls
//
//    if(_voteDataIt == _voteSnapshot.m_voteDataHistory.end() && _pollDataIt == _voteSnapshot.m_pollNumHistory.end())
//    {
//        BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment("There is currently no income to receive"));
//    }
//
//    for(; _pollDataIt != _voteSnapshot.m_pollNumHistory.end(); _pollDataIt++)
//    {
//        auto _ownedHandingfee = _voteSnapshot.m_blockSummaryHistory.find(_pollDataIt->first + 1);       // first->rounds second->summaryCooike
//        if (_pollDataIt->second <= 0 || _ownedHandingfee == _voteSnapshot.m_blockSummaryHistory.end())
//            continue;
//        _income += _ownedHandingfee->second - (_ownedHandingfee->second / 2 / _pollDataIt->second) * _pollDataIt->second;
//    }
//    for(; _voteDataIt != _voteSnapshot.m_voteDataHistory.end(); _voteDataIt++)
//    {
//        for(auto const &it : _voteDataIt->second)
//        {
//            Address _polladdr = it.first;
//            u256 _voteNum = it.second;
//            try_new_vote_snapshot(_polladdr, _blockNum);  // update polladd's snapshot
//            Account *pollAccount = account(_polladdr);
//            if(pollAccount)
//            {
//                VoteSnapshot _pollAccountvoteSnapshot = pollAccount->vote_snashot();
//                auto _pollMap = _pollAccountvoteSnapshot.m_pollNumHistory.find(_voteDataIt->first);
//                auto _handingfeeMap = _pollAccountvoteSnapshot.m_blockSummaryHistory.find(_voteDataIt->first + 1);
//                if(_pollMap == _pollAccountvoteSnapshot.m_pollNumHistory.end() || _handingfeeMap == _pollAccountvoteSnapshot.m_blockSummaryHistory.end())
//                    continue;
//                u256 _pollNum = 0;
//                u256 _handingfee = 0;
//                _pollNum = _pollMap->second;
//                if (!_pollNum)
//                    continue;
//                _handingfee = _handingfeeMap->second;
//                _income += (_handingfee / 2 / _pollNum) * _voteNum ;
//                //rounds = _voteDataIt->first;
//            }
//        }
//    }
//    if(_income)
//        addBalance(_addr, _income);
//    if (rounds != _numberofrounds)
//        a->set_numberofrounds(rounds);
//}

void State::receivingPdFeeIncome(const dev::Address &_addr, int64_t _blockNum) {
    std::pair<u256, Votingstage> _pair = config::getVotingCycle(_blockNum);
    u256 rounds = _pair.first > 0 ? _pair.first - 1 : 0;

    auto a = account(_addr);
    auto systemAccount = account(dev::PdSystemAddress);

    u256 _numofRounds = a->getFeeNumofRounds();
    VoteSnapshot _voteSnapshot = a->vote_snashot();
    CouplingSystemfee _couplingSystemfee = systemAccount->getFeeSnapshot();
    std::map<u256, std::map<Address, u256>>::const_iterator _voteDataIt = _voteSnapshot.m_voteDataHistory.find(
            _numofRounds + 1);
    u256 _BrcIncome = 0;
    u256 _CookieIncome = 0;
    for (; _voteDataIt != _voteSnapshot.m_voteDataHistory.end(); _voteDataIt++) {
        std::map<u256, std::vector<PollData>>::const_iterator _it = _couplingSystemfee.m_sorted_creaters.find(
                _voteDataIt->first);
        std::map<u256, std::pair<u256, u256>>::const_iterator _amountIt = _couplingSystemfee.m_Feesnapshot.find(
                _voteDataIt->first + 1);
        if (_amountIt == _couplingSystemfee.m_Feesnapshot.end()) {
            break;
        }
        std::vector<PollData> _PollDataV = _it->second;
        std::map<Address, u256> _superNodeAddrMap;
        u256 _totalPoll = 0;
        u256 _totalBrcFee = _amountIt->second.first;
        u256 _totalCookieFee = _amountIt->second.second;
        bool _isSuperNode = false;
        u256 _superNodePoll = 0;
        for (uint32_t i = 0; i < 7 && i < _PollDataV.size(); i++) {
            _totalPoll += _PollDataV[i].m_poll;
            if (_addr == _PollDataV[i].m_addr) {
                _isSuperNode = true;
                _superNodePoll = _PollDataV[i].m_poll;
            }
            _superNodeAddrMap[_PollDataV[i].m_addr] = _PollDataV[i].m_poll;
        }

        if (_isSuperNode == true) {
            u256 _Brc = _totalBrcFee / _totalPoll * _superNodePoll;
            u256 _Cookie = _totalCookieFee / _totalPoll * _superNodePoll;
            _BrcIncome += _Brc - (_Brc / 2 / _superNodePoll * _superNodePoll);
            _CookieIncome += _Cookie - (_Cookie / 2 / _superNodePoll * _superNodePoll);
        }
        for (auto const &_voteAddressIt : _voteDataIt->second) {
            if (_superNodeAddrMap.count(_voteAddressIt.first)) {
                std::map<Address, u256>::const_iterator _addrPollIt = _superNodeAddrMap.find(_voteAddressIt.first);
                u256 _currentNodeBrc = _totalBrcFee / _totalPoll * _addrPollIt->second;
                u256 _currentNodeCookie = _totalCookieFee / _totalPoll * _addrPollIt->second;
                _BrcIncome += (_currentNodeBrc / 2 / _addrPollIt->second * _voteAddressIt.second);
                _CookieIncome += (_currentNodeCookie / 2 / _addrPollIt->second * _voteAddressIt.second);
            }
        }
    }
    addBalance(_addr, _CookieIncome);
    addBRC(_addr, _BrcIncome);
    if (rounds) {
        a->set_numberofrounds(rounds);
    }

}

std::pair<u256, u256>
State::anytime_receivingPdFeeIncome(const dev::Address &_addr, int64_t _blockNum, bool _is_update /*true*/) {
    u256 total_income_cookies = 0;
    u256 total_income_brcs = 0;
    bool is_update = false;
    auto a = account(_addr);
    auto systemAccount = account(dev::PdSystemAddress);
    if (!a || !systemAccount) {
        BOOST_THROW_EXCEPTION(UnknownAccount() << errinfo_comment(std::string("can't get Account in received fee")));
    }
    auto received_sanp = a->getFeeSnapshot();
    auto system_sanp = systemAccount->getFeeSnapshot();
    VoteSnapshot vote_sanp = a->vote_snashot();
    std::pair<uint32_t, Votingstage> _pair = config::getVotingCycle(_blockNum);
    CouplingSystemfee fee_temp = a->getFeeSnapshot();
    /// calculate now vote_log and summary_income
    std::vector<PollData> now_miner;
    u256 now_total_poll = 0;
    uint32_t num = config::minner_rank_num();
    for (auto const &val: vote_data(SysVarlitorAddress)) {
        if (num) {
            PollData add_data = val;
            if(_blockNum >= config::newChangeHeight()){
                if (auto miner = account(val.m_addr)){
                    if (miner->mappingAddress().first != Address() && miner->mappingAddress().second == val.m_addr){
                        add_data.m_addr = miner->mappingAddress().first;
                    }
                }
            }
            now_miner.emplace_back(add_data);
            now_total_poll += add_data.m_poll;
            --num;
        }
    }
    //summary_income
    u256 now_total_cookies = systemAccount->balance();
    u256 now_total_brcs = systemAccount->BRC();
    //auto old_sunmmary = received_sanp.m_total_summary();
    std::pair<u256, u256> old_summary = received_sanp.m_total_summary;
    bool is_now_rounds = false;
    int start_round = received_sanp.m_numofrounds != 0 ? (int) received_sanp.m_numofrounds : 1;
    /// loop any not received rounds . to now rounds
    for (int i = start_round; i <= _pair.first; i++) {
        is_now_rounds = (i == _pair.first);
        u256 round_cookies = 0;
        u256 round_brcs = 0;
        u256 _totalPoll = 0;
        std::pair<u256, u256> summary = {0, 0};
        std::vector<PollData> check_creater;
        do {
            if (!vote_sanp.m_voteDataHistory.count(i - 1)) {
                break;
            }
            if (!system_sanp.m_Feesnapshot.count(i) && !is_now_rounds) {
                break;
            }
            if (!system_sanp.m_sorted_creaters.count(i - 1) && !is_now_rounds) {
                break;
            }
            if (is_now_rounds) {
                _totalPoll = now_total_poll;
                summary = std::make_pair(now_total_brcs, now_total_cookies);
                check_creater = now_miner;
            } else {
                _totalPoll = system_sanp.get_total_poll(i, config::minner_rank_num());
                summary = system_sanp.m_Feesnapshot[i];
                if(_blockNum >= config::newChangeHeight()){
                    for(int j =0; j< config::minner_rank_num() && j < system_sanp.m_sorted_creaters[i].size(); j++){
                        check_creater.emplace_back(system_sanp.m_sorted_creaters[i][j]);
                    }
                }
                else{
                    check_creater = system_sanp.m_sorted_creaters[i];
                }
            }
            if (summary.first == old_summary.first && summary.second == old_summary.second) {
                old_summary = std::make_pair(0, 0);
                continue;
            }
            if (_totalPoll == 0)
                continue;
            //vote log for other
            std::map<Address, u256> vote_log = vote_sanp.m_voteDataHistory[i - 1];
            /// loop all
            for (auto const &val: check_creater) {
                if (val.m_poll == 0)
                    continue;
                if (_addr != val.m_addr && !vote_log.count(val.m_addr)) {
                    continue;
                }
                // val = PollData
                u256 node_summary_cookies = summary.second / _totalPoll * val.m_poll;
                u256 node_summary_brcs = summary.first / _totalPoll * val.m_poll;
                u256 _income_cookies = 0;
                u256 _income_brcs = 0;
                std::pair<u256, u256> _old_get = {0, 0}; //<brc, cookies>

                if (received_sanp.m_received_cookies.count(i) &&
                    received_sanp.m_received_cookies[i].count(val.m_addr)) {
                    _old_get = received_sanp.m_received_cookies[i][val.m_addr];
                }
                if (_old_get.second >= node_summary_cookies && _old_get.first >= node_summary_brcs) {
                    continue;
                }
                if (_addr == val.m_addr) {
                    // super
                    _income_brcs += node_summary_brcs - (node_summary_brcs / 2 / val.m_poll * val.m_poll);
                    _income_cookies += node_summary_cookies - (node_summary_cookies / 2 / val.m_poll * val.m_poll);
                }
                if (vote_log.count(val.m_addr)) {
                    // vote node
                    _income_brcs += node_summary_brcs / 2 / val.m_poll * vote_log[val.m_addr];
                    _income_cookies += node_summary_cookies / 2 / val.m_poll * vote_log[val.m_addr];
                }

                fee_temp.up_received_cookies_brcs(i, val.m_addr, std::make_pair(_income_brcs, _income_cookies),
                                                  summary);

                if (_income_brcs > _old_get.first) {
                    round_brcs += (_income_brcs - _old_get.first);
                    is_update = true;
                }
                if (_income_cookies > _old_get.second) {
                    round_cookies += (_income_cookies - _old_get.second);
                    is_update = true;
                }
                old_summary = std::make_pair(0, 0);
            }
        } while (false);
        /// add received
        total_income_cookies += round_cookies;
        total_income_brcs += round_brcs;
    }
    if (is_update && _is_update) {
        fee_temp.m_numofrounds = _pair.first;
        m_changeLog.emplace_back(dev::PdSystemAddress, a->getFeeSnapshot());
        a->setCouplingSystemFeeSnapshot(fee_temp);
        a->addBRC(total_income_brcs);
        a->addBalance(total_income_cookies);
    }
    return std::make_pair(total_income_brcs, total_income_cookies);
}

void State::transferAutoEx(std::vector<std::shared_ptr<transationTool::operation>> const& _ops, h256 const& _trxid, int64_t _timeStamp, int64_t height, u256 const& _baseGas)
{
    for(auto const& val : _ops)
    {
        std::shared_ptr<transationTool::transferAutoEx_operation> _op = std::dynamic_pointer_cast<transationTool::transferAutoEx_operation>(val);
        std::shared_ptr<transationTool::pendingorder_opearaion> const& _pdop = std::make_shared<transationTool::pendingorder_opearaion>((transationTool::op_type)3, _op->m_from, ex::order_type::buy, ex::order_token_type::FUEL, ex::order_buy_type::all_price, u256(0), _op->m_autoExNum);
        std::vector<std::shared_ptr<transationTool::operation>> _pdops;
        _pdops.push_back(_pdop);
        std::pair<u256, u256> _exNumPair = pendingOrders(_op->m_from, _timeStamp, height, _trxid, _pdops);  // first: exCookieNum  second:exBRCNum
        if(_op->m_autoExType == transationTool::transferAutoExType::Balancededuction)
        {
            transferBRC(_op->m_from, _op->m_to, _op->m_transferNum);
        }else if(_op->m_autoExType == transationTool::transferAutoExType::Transferdeduction)
        {
            transferBRC(_op->m_from, _op->m_to, _op->m_transferNum - _exNumPair.second);
            transferBalance(_op->m_from, _op->m_to, _exNumPair.first - _baseGas);
        }
    }
}

void State::createContract(Address const &_address) {
    createAccount(_address, {requireAccountStartNonce(), 0});
}

void State::createAccount(Address const &_address, Account const &&_account) {
    assert(!addressInUse(_address) && "Account already exists");
    m_cache[_address] = std::move(_account);
    m_nonExistingAccountsCache.erase(_address);
    m_changeLog.emplace_back(Change::Create, _address);
}

void State::kill(Address _addr) {
    if (auto a = account(_addr))
        a->kill();
    // If the account is not in the db, nothing to kill.
}

u256 State::getNonce(Address const &_addr) const {
    if (auto a = account(_addr))
        return a->nonce();
    else
        return m_accountStartNonce;
}

u256 State::storage(Address const &_id, u256 const &_key) const {
    if (Account const *a = account(_id))
        return a->storageValue(_key, m_db);
    else
        return 0;
}

void State::setStorage(Address const &_contract, u256 const &_key, u256 const &_value) {
    m_changeLog.emplace_back(_contract, _key, storage(_contract, _key));
    m_cache[_contract].setStorage(_key, _value);
}

u256 State::originalStorageValue(Address const &_contract, u256 const &_key) const {
    if (Account const *a = account(_contract))
        return a->originalStorageValue(_key, m_db);
    else
        return 0;
}

bytes State::storageBytes(Address const& _addr, h256 const& _key) const
{
    if(Account const *a = account(_addr))
    {
        return a->storageByteValue(_key, m_db);
    }else{
        return bytes();
    }
}

void State::setStorageBytes(Address const& _addr, h256 const& _key, bytes const& _value)
{
    if(Account *a = account(_addr))
    {
        std::unordered_map<h256, bytes> _oldmap = a->storageByteOverlay();
        a->setStorageByte(_key, _value);
        m_changeLog.emplace_back(_addr,_oldmap);
    }else{
        BOOST_THROW_EXCEPTION(NotEnoughCash() << errinfo_comment(std::string("Account does not exist")));
    }
}

bytes State::originalStorageBytesValue(Address const& _addr, h256 const& _key)
{
    if (Account const *a = account(_addr))
    {
        return a->originalStorageByteValue(_key, m_db);
    }
    else
    {
        return bytes();
    }
}

void State::clearStorageByte(Address const& _addr)
{
    if(Account *a = account(_addr))
    {
        if(a->baseByteRoot() == EmptyTrie)
        {
            return;
        }else{
            m_changeLog.emplace_back(Change::StorageByteRoot, _addr, a->baseByteRoot());
            a->clearStorageByte();
        }
    }else{
        BOOST_THROW_EXCEPTION(NotEnoughCash() << errinfo_comment(std::string("Account does not exist")));
    }
}

void State::clearStorage(Address const &_contract) {
    h256 const &oldHash{m_cache[_contract].baseRoot()};
    if (oldHash == EmptyTrie)
        return;
    m_changeLog.emplace_back(Change::StorageRoot, _contract, oldHash);
    m_cache[_contract].clearStorage();
}

map<h256, pair<u256, u256>> State::storage(Address const &_id) const {
#if BRC_FATDB
    map<h256, pair<u256, u256>> ret;

    if (Account const* a = account(_id))
    {
        // Pull out all values from trie storage.
        if (h256 root = a->baseRoot())
        {
            SecureTrieDB<h256, OverlayDB> memdb(
                const_cast<OverlayDB*>(&m_db), root);  // promise we won't alter the overlay! :)

            for (auto it = memdb.hashedBegin(); it != memdb.hashedEnd(); ++it)
            {
                h256 const hashedKey((*it).first);
                u256 const key = h256(it.key());
                u256 const value = RLP((*it).second).toInt<u256>();
                ret[hashedKey] = make_pair(key, value);
            }
        }

        // Then merge cached storage over the top.
        for (auto const& i : a->storageOverlay())
        {
            h256 const key = i.first;
            h256 const hashedKey = sha3(key);
            if (i.second)
                ret[hashedKey] = i;
            else
                ret.erase(hashedKey);
        }
    }
    return ret;
#else
    (void) _id;
    BOOST_THROW_EXCEPTION(
            InterfaceNotSupported() << errinfo_interface("State::storage(Address const& _id)"));
#endif
}

h256 State::storageRoot(Address const &_id) const {
    string s = m_state.at(_id);
    if (s.size()) {
        RLP r(s);
        return r[2].toHash<h256>();
    }
    return EmptyTrie;
}

bytes const &State::code(Address const &_addr) const {
    Account const *a = account(_addr);
    if (!a || a->codeHash() == EmptySHA3)
        return NullBytes;

    if (a->code().empty()) {
        // Load the code from the backend.
        Account *mutableAccount = const_cast<Account *>(a);
        mutableAccount->noteCode(m_db.lookup(a->codeHash()));
        CodeSizeCache::instance().store(a->codeHash(), a->code().size());
    }

    return a->code();
}

void State::setCode(Address const &_address, bytes &&_code) {
    m_changeLog.emplace_back(_address, code(_address));
    if (!m_cache.count(_address))
        return;
    m_cache[_address].setCode(std::move(_code));
}

h256 State::codeHash(Address const &_a) const {
    if (Account const *a = account(_a))
        return a->codeHash();
    else
        return EmptySHA3;
}

size_t State::codeSize(Address const &_a) const {
    if (Account const *a = account(_a)) {
        if (a->hasNewCode())
            return a->code().size();
        auto &codeSizeCache = CodeSizeCache::instance();
        h256 codeHash = a->codeHash();
        if (codeSizeCache.contains(codeHash))
            return codeSizeCache.get(codeHash);
        else {
            size_t size = code(_a).size();
            codeSizeCache.store(codeHash, size);
            return size;
        }
    } else
        return 0;
}

size_t State::savepoint() const {
    return m_changeLog.size();
}

void State::rollback(size_t _savepoint) {
    while (_savepoint != m_changeLog.size()) {
        auto &change = m_changeLog.back();
        auto &account = m_cache[change.address];
        cdebug << BrcYellow "rollback: "
               << " change.kind" << (size_t) change.kind << " change.value:" << change.value << " address "
               << change.address
               << BrcReset << std::endl;
        // Public State API cannot be used here because it will add another
        // change log entry.
        switch (change.kind) {
            case Change::Storage:
                account.setStorage(change.key, change.value);
                break;
            case Change::StorageRoot:
                account.setStorageRoot(change.value);
                break;
            case Change::Balance:
                account.addBalance(0 - change.value);
                break;
            case Change::BRC:
                account.addBRC(0 - change.value);
                break;
            case Change::Nonce:
                account.setNonce(change.value);
                break;
            case Change::Create:
                m_cache.erase(change.address);
                break;
            case Change::Code:
                account.setCode(std::move(change.oldCode));
                break;
            case Change::Touch:
                account.untouch();
                m_unchangedCacheEntries.emplace_back(change.address);
                break;
            case Change::Ballot:
                account.addBallot(0 - change.value);
                break;
            case Change::Poll:
                account.addPoll(0 - change.value);
                break;
            case Change::Vote:
                account.addVote(change.vote);
                break;
            case Change::SysVoteData:
                account.manageSysVote(change.sysVotedate.first, !change.sysVotedate.second, 0);
                break;
            case Change::FBRC:
                account.addFBRC(0 - change.value);
                break;
            case Change::FBalance:
                account.addFBalance(0 - change.value);
                break;
            case Change::BlockReward:
                account.addBlockRewardRecoding(change.blockReward);
                break;
            case Change::NewVoteSnapshot:
                account.set_vote_snapshot(change.vote_snapshot);
                break;
            case Change::CooikeIncomeNum:
                account.addCooikeIncome(0 - change.value);
                break;
            case Change::SystemAddressPoll:
                account.set_system_poll(change.poll_data);
                break;
            case Change::CoupingSystemFeeSnapshot:
                account.setCouplingSystemFeeSnapshot(change.feeSnapshot);
                break;
            case Change::MinnerSnapshot:
                account.set_vote_data(change.minners);
                break;
            case Change::ReceiveCookies:
                account.set_received(change.received);
                break;
            case Change::UpExOrder:
                account.setExOrderMulti(change.ex_multi);
                break;
            case Change::SuccessOrder:
                account.setSuccessOrder(change.ret_orders);
                break;
            case Change::ChangeMiner:
                account.kill();
                account.copyByAccount(change.old_account);
                break;
            case Change::StorageByte:
                account.setBytetoStorage(change.storageByte);
                break;
            case Change::StorageByteRoot:
                account.setStorageBytesRoot(change.value);
                break;
            case Change::DeleteStorgaeByte:
                account.setDeleteStorageByte(change.deleteStorageByte);
            case Change::NewChangeMiner:
                account.setChangeMiner(change.mapping);
                break;
            default:
                break;
        }
        m_changeLog.pop_back();
    }
}

std::pair<ExecutionResult, TransactionReceipt> State::execute(EnvInfo const &_envInfo,
                                                              SealEngineFace const &_sealEngine, Transaction const &_t,
                                                              Permanence _p, OnOpFunc const &_onOp) {
    // Create and initialize the executive. This will throw fairly cheaply and quickly if the
    // transaction is bad in any way.
    Executive e(*this, _envInfo, _sealEngine);
    ExecutionResult res;
    e.setResultRecipient(res);

    auto onOp = _onOp;
#if BRC_VMTRACE
    if (!onOp)
        onOp = e.simpleTrace();
#endif
    u256 const startGasUsed = _envInfo.gasUsed();
    bool const statusCode = executeTransaction(e, _t, onOp);

    bool removeEmptyAccounts = false;
    switch (_p) {
        case Permanence::Reverted:
            m_cache.clear();
            break;
        case Permanence::Committed:
            removeEmptyAccounts = _envInfo.number() >= _sealEngine.chainParams().EIP158ForkBlock;
            commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts :
                   State::CommitBehaviour::KeepEmptyAccounts);
            break;
        case Permanence::Uncommitted:
            break;
    }

    TransactionReceipt const receipt = _envInfo.number() >= _sealEngine.chainParams().byzantiumForkBlock ?
                                       TransactionReceipt(statusCode, startGasUsed + e.gasUsed(), e.logs()) :
                                       TransactionReceipt(rootHash(), startGasUsed + e.gasUsed(), e.logs());
    return make_pair(res, receipt);
}

void State::executeBlockTransactions(Block const &_block, unsigned _txCount,
                                     LastBlockHashesFace const &_lastHashes, SealEngineFace const &_sealEngine) {
    u256 gasUsed = 0;
    for (unsigned i = 0; i < _txCount; ++i) {
        EnvInfo envInfo(_block.info(), _lastHashes, gasUsed);

        Executive e(*this, envInfo, _sealEngine);
        executeTransaction(e, _block.pending()[i], OnOpFunc());

        gasUsed += e.gasUsed();
    }
}

/// @returns true when normally halted; false when exceptionally halted; throws when internal VM
/// exception occurred.
bool State::executeTransaction(Executive &_e, Transaction const &_t, OnOpFunc const &_onOp) {
    size_t const savept = savepoint();
    try {
        _e.initialize(_t);

        if (!_e.execute())
            _e.go(_onOp);
        return _e.finalize();
    }
    catch (Exception const &) {
        cdebug << "rollback tx id " << toHex(_t.sha3()) << " rlp : " << toHex(_t.rlp());
        rollback(savept);
        throw;
    }
}

u256 dev::brc::State::poll(Address const &_addr) const {
    if (auto a = account(_addr))
        return a->poll();
    else
        return 0;
}

void dev::brc::State::addPoll(Address const &_addr, u256 const &_value) {
    if (Account *a = account(_addr)) {
        a->addPoll(_value);
    } else
        BOOST_THROW_EXCEPTION(InvalidAddressAddr() << errinfo_interface("State::addPoll()"));

    if (_value)
        m_changeLog.emplace_back(Change::Poll, _addr, _value);
}


void dev::brc::State::subPoll(Address const &_addr, u256 const &_value) {
    if (_value == 0)
        return;
    Account *a = account(_addr);
    if (!a || a->poll() < _value)
        BOOST_THROW_EXCEPTION(NotEnoughPoll());
    addPoll(_addr, 0 - _value);
}

void dev::brc::State::execute_vote(Address const &_addr,
                                   std::vector<std::shared_ptr<transationTool::operation> > const &_ops,
                                   EnvInfo const &info) {

    u256 block_num = (u256) info.number();
    for (auto const &val : _ops) {
        std::shared_ptr<transationTool::vote_operation> _p = std::dynamic_pointer_cast<transationTool::vote_operation>(
                val);
        if (!_p) {
            cerror << "execute vote dynamic type field!";
            BOOST_THROW_EXCEPTION(InvalidDynamic());
        }
        VoteType _type = (VoteType) _p->m_vote_type;
        if (_type == VoteType::EBuyVote) {
            trySnapshotWithMinerMapping(_addr, block_num);
            transferBallotBuy(_addr, _p->m_vote_numbers);
        } else if (_type == VoteType::ESellVote) {
            trySnapshotWithMinerMapping(_addr, block_num);
            transferBallotSell(_addr, _p->m_vote_numbers);
        } else if (_type == VoteType::ELoginCandidate) {
            trySnapshotWithMinerMapping(_addr, block_num);
            addSysVoteDate(SysElectorAddress, _addr);
        } else if (_type == VoteType::ELogoutCandidate) {
            trySnapshotWithMinerMapping(_addr, block_num);
            subSysVoteDate(SysElectorAddress, _addr);
        } else if (_type == VoteType::EDelegate) {
            trySnapshotWithMinerMapping(_addr, block_num);
            trySnapshotWithMinerMapping(_p->m_to, block_num);
            add_vote(_addr, {_p->m_to, (u256) _p->m_vote_numbers, info.timestamp()});
        } else if (_type == EUnDelegate) {
            trySnapshotWithMinerMapping(_addr, block_num);
            trySnapshotWithMinerMapping(_p->m_to, block_num);
            sub_vote(_addr, {_p->m_to, (u256) _p->m_vote_numbers, info.timestamp()});
        }
    }
}

Json::Value dev::brc::State::accoutMessage(Address const &_addr) {
    Json::Value jv;
    if (auto a = account(_addr)) {
        jv["Address"] = toJS(_addr);
        jv["balance"] = toJS(a->balance());
        jv["FBalance"] = toJS(a->FBalance());
        jv["BRC"] = toJS(a->BRC());
        jv["FBRC"] = toJS(a->FBRC());
        jv["vote"] = toJS(a->voteAll());
        jv["ballot"] = toJS(a->ballot());
        jv["poll"] = toJS(a->poll());
        jv["nonce"] = toJS(a->nonce());
        jv["cookieinsummury"] = toJS(a->CookieIncome());

        Json::Value _array;
        uint32_t num = 0;
        for (auto val : a->vote_data()) {
            if (num++ > config::max_message_num())   // limit message num
                break;
            Json::Value _v;
            _v["Address"] = toJS(val.m_addr);
            _v["vote_num"] = toJS(val.m_poll);
            _array.append(_v);
        }
        jv["vote"] = _array;

//        Json::Value record;
//        record["time"] = toJS(a->last_records(_addr));
//        jv["last_block_created"] = record;
    }

    return jv;
}


// Return to the block reward record in the form of paging
Json::Value
dev::brc::State::blockRewardMessage(Address const &_addr, uint32_t const &_pageNum, uint32_t const &_listNum) {
    try {
        Json::Value jv;
        if (auto a = account(_addr)) {
            if (a->blockReward().size() > 0) {
                std::vector<std::pair<u256, u256>> _Vector = a->blockReward();
                uint32_t _retList = 0;
                if (_pageNum * _listNum > _Vector.size()) {
                    _retList = _Vector.size();
                } else {
                    _retList = _pageNum * _listNum;
                }
                if (_pageNum == 0 || _listNum == 0) {
                    return jv;
                }
                if ((_pageNum - 1) * _listNum > _Vector.size()) {
                    return jv;
                }

                std::vector<std::pair<u256, u256>> res(_retList - ((_pageNum - 1) * _listNum));

                if (_pageNum == 1) {
                    std::copy(_Vector.begin(), _Vector.begin() + _retList, res.begin());
                } else {
                    std::copy(_Vector.begin() + (_pageNum - 1) * _listNum, _Vector.begin() + _retList, res.begin());
                }
                Json::Value _rewardArray;
                for (auto it : res) {
                    Json::Value _vReward;
                    _vReward["blockNum"] = toJS(it.first);
                    _vReward["rewardNum"] = toJS(it.second);
                    _rewardArray.append(_vReward);
                }
                jv["BlockReward"] = _rewardArray;
            }
        }
        return jv;
    } catch (...) {
        throw;
    }

}

Json::Value dev::brc::State::votedMessage(Address const &_addr) const {
    Json::Value jv;
    if (auto a = account(_addr)) {
        Json::Value _arry;
        int _num = 0;
        uint32_t num = 0;
        for (auto val : a->vote_data()) {
            if (num++ > config::max_message_num())   // limit message num
                break;
            Json::Value _v;
            _v["address"] = toJS(val.m_addr);
            _v["voted_num"] = toJS(val.m_poll);
            _arry.append(_v);
            _num += (int) val.m_poll;
        }
        jv["vote"] = _arry;
        jv["total_voted_num"] = toJS(_num);
    }
    return jv;
}

Json::Value dev::brc::State::electorMessage(Address _addr) const {
    Json::Value jv;
    Json::Value _arry;
    const std::vector<PollData> _data = vote_data(SysElectorAddress);
    if (_addr == ZeroAddress) {
        int num = 0;
        for (auto val : _data) {
            if (num++ > config::max_message_num())   // limit message num
                break;
            Json::Value _v;
            _v["address"] = toJS(val.m_addr);
            _v["vote_num"] = toJS(val.m_poll);
            _arry.append(_v);
        }
        jv["electors"] = _arry;
    } else {
        jv["addrsss"] = toJS(_addr);
        auto ret = std::find(_data.begin(), _data.end(), _addr);
        auto a = account(_addr);
        if (ret != _data.end() && a) {
            jv["obtain_vote"] = toJS(a->poll());
        } else
            jv["ret"] = "not is the eletor";
    }
    return jv;
}

Account dev::brc::State::systemPendingorder(int64_t _time) {
    auto u256Safe = [](std::string const &s) -> u256 {
        bigint ret(s);
        if (ret >= bigint(1) << 256)
            BOOST_THROW_EXCEPTION(
                    ValueTooLarge() << errinfo_comment("State value is equal or greater than 2**256"));
        return (u256) ret;
    };
    ExdbState _exdbState(*this);
    auto a = account(dev::VoteAddress);
    std::string _num = "1450000000000000";
    cwarn << "genesis pendingorder Num :" << _num;
    u256 systenCookie = u256Safe(_num);
    ex_order _order = {h256(1), dev::systemAddress, u256Safe(std::string("100000000")), systenCookie, systenCookie,
                       _time, dev::brc::ex::order_type::sell, dev::brc::ex::order_token_type::FUEL,
                       dev::brc::ex::order_buy_type::only_price};
    cerror << " system time  = " << _time << "  price = :" << u256Safe(std::string("100000000"));
    try {
        _exdbState.insert_operation(_order);
    }
    catch (const boost::exception &e) {
        cwarn << boost::diagnostic_information_what(e);
        exit(1);
    }
    catch (...) {
        exit(1);
    }
//	m_exdb.commit(1, h256(), h256());
//    m_exdb.rollback();
//    m_exdb.commit_disk(1, true);
//	cnote << m_exdb.check_version(false);
    return *account(ExdbSystemAddress);
}

void dev::brc::State::add_vote(const dev::Address &_id, dev::brc::PollData const &p_data) {
    Address _recivedAddr = p_data.m_addr;
    u256 _value = p_data.m_poll;
    Account *a = account(_id);
    Account *rec_a = account(_recivedAddr);
    Account *sys_a = account(SysElectorAddress);
    u256 _poll = 0;
    PollData poll_data;
    if (a && rec_a && sys_a) {
        if (a->ballot() < _value)
            BOOST_THROW_EXCEPTION(NotEnoughBallot() << errinfo_interface("State::addvote()"));
        _poll = rec_a->poll();
        poll_data = sys_a->poll_data(_recivedAddr);

        a->addBallot(0 - _value);
        a->addVote(std::make_pair(_recivedAddr, _value));
        rec_a->addPoll(_value);
        sys_a->set_system_poll({_recivedAddr, rec_a->poll(), poll_data.m_time > 0 ? poll_data.m_time : p_data.m_time});
    } else
        BOOST_THROW_EXCEPTION(InvalidAddressAddr() << errinfo_interface("State::addvote()"));

    if (_value) {
        m_changeLog.emplace_back(Change::Vote, _id, std::make_pair(_recivedAddr, 0 - _value));
        m_changeLog.emplace_back(Change::SystemAddressPoll, _id, poll_data);
        m_changeLog.emplace_back(Change::Ballot, _id, 0 - _value);
        m_changeLog.emplace_back(Change::Poll, _recivedAddr, _value);
    }
}

void dev::brc::State::sub_vote(const dev::Address &_id, dev::brc::PollData const &p_data) {
    Address _recivedAddr = p_data.m_addr;
    u256 _value = p_data.m_poll;
    Account *rec_a = account(_recivedAddr);
    Account *a = account(_id);
    Account *sys_a = account(SysElectorAddress);
    u256 _poll;
    PollData poll_data;

    if (a && rec_a && sys_a) {
        // 验证投票将记录
        if (a->poll_data(_recivedAddr).m_poll < _value) {
            cerror << "not has enough tickets...";
            BOOST_THROW_EXCEPTION(NotEnoughVoteLog() << errinfo_interface("State::subVote()"));
        }
        _poll = rec_a->poll();
        poll_data = sys_a->poll_data(_recivedAddr);

        if (rec_a->poll() < _value)
            _value = rec_a->poll();
        rec_a->addPoll(0 - _value);
        a->addVote(std::make_pair(_recivedAddr, 0 - _value));
        a->addBallot(_value);
        sys_a->set_system_poll({_recivedAddr, rec_a->poll(), poll_data.m_time > 0 ? poll_data.m_time : p_data.m_time});
    } else {
        cerror << "address error!";
        BOOST_THROW_EXCEPTION(InvalidAddressAddr() << errinfo_interface("State::subVote()"));
    }

    if (_value) {
        m_changeLog.emplace_back(Change::Vote, _id, std::make_pair(_recivedAddr, 0 - _value));
        m_changeLog.emplace_back(Change::SystemAddressPoll, _id, poll_data);
        m_changeLog.emplace_back(Change::Ballot, _id, _value);
        m_changeLog.emplace_back(Change::Poll, _recivedAddr, 0 - _value);
    }
}

const std::vector<PollData> dev::brc::State::vote_data(const dev::Address &_addr) const {
    if (auto a = account(_addr)) {
        return a->vote_data();
    }
    return std::vector<PollData>();
}

const PollData dev::brc::State::poll_data(const dev::Address &_addr, const dev::Address &_recv_addr) const {
    if (auto a = account(_addr)) {
        return a->poll_data(_recv_addr);
    }
    return PollData();
}

std::pair<Address, Address>  dev::brc::State::minerMapping(Address const& addr){
    if (auto a = account(addr)){
        return a->mappingAddress();
    }
    return  {Address(), Address()};
}

void dev::brc::State::addSysVoteDate(Address const &_sysAddress, Address const &_id) {
    Account *sysAddr = account(_sysAddress);
    Account *a = account(_id);
    if (!sysAddr) {
        createAccount(_sysAddress, {requireAccountStartNonce(), 0});
        sysAddr = account(_sysAddress);
    }
    if (!a)
        BOOST_THROW_EXCEPTION(InvalidAddressAddr() << errinfo_interface("State::addSysVoteDate()"));
    sysAddr->manageSysVote(_id, true, 0);
    m_changeLog.emplace_back(_sysAddress, std::make_pair(_id, true));
}


void dev::brc::State::subSysVoteDate(Address const &_sysAddress, Address const &_id) {
    Account *sysAddr = account(_sysAddress);
    Account *a = account(_id);
    if (!sysAddr)
        BOOST_THROW_EXCEPTION(InvalidSysAddress() << errinfo_interface("State::subSysVoteDate()"));
    if (!a)
        BOOST_THROW_EXCEPTION(InvalidAddressAddr() << errinfo_interface("State::subSysVoteDate()"));
    sysAddr->manageSysVote(_id, false, a->poll(), 0);
    m_changeLog.emplace_back(_sysAddress, std::make_pair(_id, false));
}

void dev::brc::State::trySnapshotWithMinerMapping(const dev::Address &_addr, dev::u256 _block_num) {
    std::pair<uint32_t, Votingstage> _pair = dev::brc::config::getVotingCycle((int64_t) _block_num);
    if (_pair.second == Votingstage::ERRORSTAGE)
        return;

    // TODO frok code
    if(_block_num >= config::newChangeHeight()) {
        auto miner_mapping = minerMapping(_addr);
        if (miner_mapping.first != Address() && miner_mapping.first != _addr && miner_mapping.second == _addr) {
            try_new_vote_snapshot(miner_mapping.first, _block_num);
        }
    }
    try_new_vote_snapshot(_addr, _block_num);
}

void dev::brc::State::try_new_vote_snapshot(const dev::Address &_addr, dev::u256 _block_num) {
    std::pair<uint32_t, Votingstage> _pair = dev::brc::config::getVotingCycle((int64_t) _block_num);
    if (_pair.second == Votingstage::ERRORSTAGE)
        return;
    auto a = account(_addr);
    if (! a) {
        createAccount(_addr, {0});
        a = account(_addr);
    }

    std::pair<bool, u256> ret_pair = a->get_no_record_snapshot((u256) _pair.first, _pair.second);
    if (!ret_pair.first) {
        return;
    }
    VoteSnapshot _vote_sna = a->vote_snashot();
    /// try new snapshot
    a->try_new_snapshot(ret_pair.second);

    /// clear genesis_vote_data and genesis_rounds poll
    if (_vote_sna.m_latest_round == 0) {
        std::vector<PollData> poll_data = a->vote_data();
        a->clear_vote_data();
        subPoll(_addr, a->poll());
        m_changeLog.emplace_back(Change::MinnerSnapshot, _addr, poll_data);
    }

    m_changeLog.emplace_back(_addr, _vote_sna);
    m_changeLog.emplace_back(Change::CooikeIncomeNum, _addr, 0 - a->CookieIncome());

    setCookieIncomeNum(_addr, 0);
}

void dev::brc::State::tryRecordFeeSnapshot(int64_t _blockNum) {
    std::pair<uint32_t, Votingstage> _pair = config::getVotingCycle(_blockNum);
    auto a = account(dev::PdSystemAddress);
    if (!a) {
        createAccount(dev::PdSystemAddress, {0});
        a = account(dev::PdSystemAddress);
    }
    u256 _rounds = a->getSnapshotRounds();
    if (_pair.first > _rounds) {
        CouplingSystemfee _fee = a->getFeeSnapshot();

        auto ret_fee = a->getFeeSnapshot().m_sorted_creaters.find(_rounds);
        if (_rounds != 0 && (ret_fee == a->getFeeSnapshot().m_sorted_creaters.end() || ret_fee->second.empty())) {
            return;
        }
        u256 total_poll = a->getFeeSnapshot().get_total_poll(_rounds);
        if (total_poll == 0 && _pair.first > 2) {
            return;
        }

        u256 remainder_brc = 0;
        u256 remainder_ballance = 0;

        std::vector<PollData> _pollDataV = vote_data(SysVarlitorAddress);
        u256 _snapshotTotalPoll = 0;
        for (uint32_t i = 0; i < config::minner_rank_num() && i < _pollDataV.size(); i++) {
            _snapshotTotalPoll += _pollDataV[i].m_poll;
        }

        if (_snapshotTotalPoll != 0) {
            remainder_brc = a->BRC() % _snapshotTotalPoll;
            remainder_ballance = a->balance() % _snapshotTotalPoll;
        }

        auto vote_datas = vote_data(SysVarlitorAddress);
        /// fork code about changeMiner
        if (_blockNum >= config::newChangeHeight()) {
            for (auto &d: vote_datas) {
                auto miner_mapping = minerMapping(d.m_addr);
                if (miner_mapping.first != Address()) {
                    d.m_addr = miner_mapping.first;
                }
            }
        }

        a->tryRecordSnapshot(_pair.first, a->BRC() - remainder_brc, a->balance() - remainder_ballance,
                             vote_datas, _blockNum);

        setBRC(dev::PdSystemAddress, remainder_brc);
        setBalance(dev::PdSystemAddress, remainder_ballance);
        m_changeLog.emplace_back(dev::PdSystemAddress, _fee);
    }
    return;
}

void dev::brc::State::transferBallotBuy(Address const &_from, u256 const &_value) {
    subBRC(_from, _value * BALLOTPRICE);
    addBRC(dev::systemAddress, _value * BALLOTPRICE);
    addBallot(_from, _value);
}

void dev::brc::State::transferBallotSell(Address const &_from, u256 const &_value) {
    subBallot(_from, _value);
    addBRC(_from, _value * BALLOTPRICE);
    subBRC(dev::systemAddress, _value * BALLOTPRICE);
}

int64_t dev::brc::State::last_block_record(Address const &_id) const {
    auto a = account(SysBlockCreateRecordAddress);
    if (!a) {
        return 0;
    }
    return a->last_records(_id);

}

void dev::brc::State::set_last_block_record(const dev::Address &_id, std::pair<int64_t, int64_t> value,
                                            uint32_t varlitor_time) {
    auto a = account(SysBlockCreateRecordAddress);
    if (!a) {
        createAccount(SysBlockCreateRecordAddress, {0});
        a = account(SysBlockCreateRecordAddress);
    }
    int64_t _time = a->last_records(_id);
    a->set_create_record(std::make_pair(_id, value.second));

    if (value.first == 1) {
        // if the first block will record all super_minner
        // bese for _id's offset to set other-time
        if (auto varlitor_a = account(SysVarlitorAddress)) {
            // find offset
            int offset = 0;
            for (auto const &val: varlitor_a->vote_data()) {
                if (val.m_addr == _id)
                    break;
                ++offset;
            }
            int index = 0;
            for (auto const &val: varlitor_a->vote_data()) {
                if (val.m_addr != _id) {
                    int64_t _time = value.second + (index - offset) * (int) varlitor_time;
                    a->set_create_record(std::make_pair(val.m_addr, _time));
                }
                ++index;
            }
        }
    }
    m_changeLog.emplace_back(Change::LastCreateRecord, _id, std::make_pair(_id, _time));
}

BlockRecord dev::brc::State::block_record() const {
    auto a = account(SysBlockCreateRecordAddress);
    if (!a)
        return BlockRecord();
    return a->block_record();
}
std::pair<Address, Address> dev::brc::State::replaceMiner(Address const& _id) const{
    auto a = account(_id);
    if (a)
        return a->mappingAddress();
    return std::make_pair(Address(), Address());
}

void dev::brc::State::try_newrounds_count_vote(const dev::brc::BlockHeader &curr_header,
                                               const dev::brc::BlockHeader &previous_header) {
    //testlog << "curr:"<< curr_header.number() << "  pre:"<< previous_header.number();
    std::pair<uint32_t, Votingstage> previous_pair = dev::brc::config::getVotingCycle(previous_header.number());
    std::pair<uint32_t, Votingstage> curr_pair = dev::brc::config::getVotingCycle(curr_header.number());

    if (curr_pair.first <= previous_pair.first)
        return;
    //testlog << "start to new rounds";
    // add minnner_snapshot
    tryRecordFeeSnapshot(curr_header.number());

    //will countVote and replace creater
    auto a = account(SysElectorAddress);
    if (!a)
        return;
    auto varlitor_a = account(SysVarlitorAddress);
    if (!varlitor_a) {
        createAccount(SysVarlitorAddress, {0});
        varlitor_a = account(SysVarlitorAddress);
    }
    auto standby_a = account(SysCanlitorAddress);
    if (!standby_a) {
        createAccount(SysCanlitorAddress, {0});
        standby_a = account(SysCanlitorAddress);
    }

    std::vector<PollData> p_data = a->vote_data();
    PollData::sort_greater(p_data);

    u256 var_num = config::varlitorNum();
    u256 standby_num = config::standbyNum();
    std::vector<PollData> varlitors;
    std::vector<PollData> standbys;

    if(curr_header.number() >= config::newChangeHeight()) {
        /// fork code about newChangeMiner
        for (auto &val: p_data) {
            auto mapping_addr = minerMapping(val.m_addr);
            if(mapping_addr.first !=Address() && mapping_addr.second == val.m_addr){
                val.m_addr = mapping_addr.first;
            }
            if (var_num) {
                varlitors.emplace_back(val);
                var_num--;
            } else if (standby_num) {
                standbys.emplace_back(val);
                standby_num--;
            }
        }
    }
    else {
        for (auto const &val: p_data) {
            if (var_num) {
                varlitors.emplace_back(val);
                var_num--;
            } else if (standby_num) {
                standbys.emplace_back(val);
                standby_num--;
            }
        }
    }

    if (varlitors.empty())
        return;

    ///add sanpshot about miner and standby
    auto minersanp_a = account(SysMinerSnapshotAddress);
    if (!minersanp_a) {
        createAccount(SysMinerSnapshotAddress, {0});
        minersanp_a = account(SysMinerSnapshotAddress);
    }
    std::vector<PollData> _v = varlitor_a->vote_data();
    std::vector<PollData> _v1 = standby_a->vote_data();
    _v.insert(_v.end(), _v1.begin(), _v1.end());

    m_changeLog.emplace_back(SysMinerSnapshotAddress, minersanp_a->getFeeSnapshot());
    minersanp_a->add_new_rounds_miner_sapshot(previous_pair.first, _v);

    if (previous_pair.first > dev::brc::config::getVotingCycle(0).first) {
        m_changeLog.emplace_back(Change::MinnerSnapshot, SysVarlitorAddress, varlitor_a->vote_data());
        m_changeLog.emplace_back(Change::MinnerSnapshot, SysCanlitorAddress, standby_a->vote_data());
        varlitor_a->set_vote_data(varlitors);
        standby_a->set_vote_data(standbys);
    }
}

std::map<u256, std::vector<PollData>> dev::brc::State::get_miner_snapshot() const {
    auto minersanp_a = account(SysMinerSnapshotAddress);
    if (!minersanp_a) {
        return std::map<u256, std::vector<PollData>>();
    }
    return minersanp_a->getFeeSnapshot().m_sorted_creaters;
}


void dev::brc::State::addExchangeOrder(Address const &_addr, dev::brc::ex::ex_order const &_order) {
    Account *_account = account(_addr);
    if (!_account) {
        BOOST_THROW_EXCEPTION(
                ExdbChangeFailed() << errinfo_comment(std::string("addExchangeOrder failed: account is not exist")));
    }
    dev::brc::ex::ExOrderMulti _oldMulti = _account->getExOrder();

    _account->addExOrderMulti(_order);

    m_changeLog.emplace_back(Change::UpExOrder, _addr, _oldMulti);
}


void dev::brc::State::newAddExchangeOrder(Address const& _addr, dev::brc::ex::ex_order const& _order)
{
    Address _orderAddress;
    if(_order.type == ex::order_type::buy)
    {
        _orderAddress = dev::BuyExchangeAddress;
    }else if(_order.type == ex::order_type::sell)
    {
        _orderAddress = dev::SellExchangeAddress;
    }else{
         BOOST_THROW_EXCEPTION(ExdbChangeFailed() << errinfo_comment(std::string("Order transaction type analysis error")));
    }

    Account *_orderAccount = account(_orderAddress);
    if(!_orderAccount)
    {
        createAccount(_orderAddress, {0});
        _orderAccount = account(_orderAddress);
    }

    std::unordered_map<h256, bytes> _oldmap = _orderAccount->storageByteOverlay();
    h256 _oldroot = _orderAccount->baseByteRoot();
    _orderAccount->exchangeBplusAdd(_order, m_db);

    m_changeLog.emplace_back(_orderAddress, _oldmap);
    m_changeLog.emplace_back(Change::StorageByteRoot, _orderAddress, _oldroot);
}

Json::Value dev::brc::State::newExorderGet(int64_t const& _time, u256 const& _price)
{
    Account *_account = account(dev::TestbplusAddress);
    if(!_account)
    {
        BOOST_THROW_EXCEPTION(  
                ExdbChangeFailed() << errinfo_comment(std::string("addExchangeOrder failed: account is not exist")));
    }

    std::pair<bool, dev::brc::exchangeValue> _ret = _account->exchangeBplusGet(_price, _time, m_db);
    if(_ret.first)
    {
        Json::Value _retJson;
        _retJson["orderID"] = toJS(_ret.second.m_orderId);
        _retJson["from"] = toJS(_ret.second.m_from);
        _retJson["pendingorderNum"] = toJS(_ret.second.m_pendingorderNum);
        _retJson["pendingordertokenNum"] = toJS(_ret.second.m_pendingordertokenNum);
        _retJson["pendingorderPrice"] = toJS(_ret.second.m_pendingorderPrice);
        _retJson["createTime"] = toJS(_ret.second.m_createTime);
        std::tuple<std::string, std::string, std::string>  _t = enumToString(_ret.second.m_pendingorderType,_ret.second.m_pendingorderTokenType,_ret.second.m_pendingorderBuyType); 
        _retJson["pendingorderType"] = std::get<0>(_t);
        _retJson["pendingorderTokenType"] = std::get<1>(_t);
        _retJson["pendingorderBuyType"] = std::get<2>(_t);;
        return _retJson;
    }else{
        return Json::Value();
    }
}

// Json::Value dev::brc::State::newExorderAllGet()
// {
//     Account *_account = account(dev::TestbplusAddress);
//     if(!_account)
//     {
//         BOOST_THROW_EXCEPTION(  
//                 ExdbChangeFailed() << errinfo_comment(std::string("addExchangeOrder failed: account is not exist")));
//     }
//     Json::Value _ret = _account->exchangeBplusAllGet(m_db);
//     return _ret;
// }

Json::Value dev::brc::State::newExorderGetByType( uint8_t _order_type){ 
    Address orderAddress;
    if(_order_type == (uint8_t)dev::brc::ex::order_type::buy){
        orderAddress = dev::BuyExchangeAddress;
    }
    else if(_order_type == (uint8_t)dev::brc::ex::order_type::sell){
         orderAddress = dev::SellExchangeAddress;
    }
    else{
        BOOST_THROW_EXCEPTION(  
                ExdbChangeFailed() << errinfo_comment(std::string("addExchangeOrder failed: buyType is not exist")));
    }
    Account *_account = account(orderAddress);
    if(!_account)
    {
        BOOST_THROW_EXCEPTION(  
                ExdbChangeFailed() << errinfo_comment(std::string("addExchangeOrder failed: account is not exist")));
    }
    Json::Value _ret;
    if(_order_type == (uint8_t)ex::order_type::buy)
    {
        _ret = _account->exchangeBplusBuyAllGet( m_db);
    }else{
        _ret = _account->exchangeBplusSellAllGet( m_db);
    }
    return _ret;
}


std::pair<sellOrder::iterator, sellOrder::iterator> dev::brc::State::newGetSellExChangeOrder(int64_t const& _time, u256 const& _price)
{
    Account *_orderAccount = account(dev::SellExchangeAddress);
    if(!_orderAccount)
    {
        createAccount(dev::SellExchangeAddress, {0});
        _orderAccount =  account(dev::SellExchangeAddress);
    }
    auto _ret = _orderAccount->sellExchangeGetIt(_price, _time, m_db);
    return _ret;
}


std::pair<buyOrder::iterator, buyOrder::iterator> dev::brc::State::newGetBuyExchangeOrder(int64_t const& _time, u256 const& _price)
{
    Account *_orderAccount = account(dev::BuyExchangeAddress);
    if(!_orderAccount)
    {
        createAccount(dev::BuyExchangeAddress, {0});
        _orderAccount = account(dev::BuyExchangeAddress);
    }
    auto _ret = _orderAccount->buyExchangeGetIt(_price, _time, m_db);
    return _ret;
}

void dev::brc::State::newRemoveExchangeOrder(uint8_t _orderType,int64_t const& _time, u256 const& _price, h256 const& _hash)
{
    Address _orderAddress;
    if((order_type)_orderType == ex::order_type::buy)
    {
        _orderAddress = dev::BuyExchangeAddress;
    }else if((order_type)_orderType == ex::order_type::sell)
    {
        _orderAddress = dev::SellExchangeAddress;
    }else{
        BOOST_THROW_EXCEPTION(ExdbChangeFailed() << errinfo_comment(std::string("Order transaction type analysis error")));
    }

    Account *_orderAccount = account(_orderAddress);
    if(!_orderAccount)
    {
        BOOST_THROW_EXCEPTION(ExdbChangeFailed() << errinfo_comment(std::string("Pending order address does not exist, delete order is wrong")));
    }
    std::vector<h256> _old = _orderAccount->getDeleteByte();
    _orderAccount->exchangeBplusDelete(_orderType, _time, _price, _hash, m_db);
    m_changeLog.emplace_back(_orderAddress, _old);

}

void dev::brc::State::removeExchangeOrder(const dev::Address &_addr, dev::h256 _trid) {
    Account *_account = account(_addr);
    if (!_account) {
        BOOST_THROW_EXCEPTION(
                ExdbChangeFailed() << errinfo_comment(std::string("removeExchangeOrder failed: account is not exist")));
    }
    dev::brc::ex::ExOrderMulti _oldMulti = _account->getExOrder();

    bool status = _account->removeExOrderMulti(_trid);

    if (status == false) {
        BOOST_THROW_EXCEPTION(ExdbChangeFailed() << errinfo_comment(
                std::string("removeExchangeOrder failed: cancelpendingorder error")));
    }
    //  TO DO
    m_changeLog.emplace_back(Change::UpExOrder, _addr, _oldMulti);
}

dev::brc::ex::ExOrderMulti const &dev::brc::State::getExOrder() {
    Account *_orderAccount = account(dev::ExdbSystemAddress);
    if (!_orderAccount) {
        createAccount(dev::ExdbSystemAddress, {0});
        _orderAccount = account(dev::ExdbSystemAddress);
    }

    return _orderAccount->getExOrder();
}

dev::brc::ex::ExOrderMulti const &dev::brc::State::userGetExOrder(Address const &_addr) {
    Account *_account = account(_addr);
    if (!_account) {
        BOOST_THROW_EXCEPTION(
                ExdbChangeFailed() << errinfo_comment(std::string("userGetExOrder failed : account is not exist")));
    }

    return _account->getExOrder();
}

void dev::brc::State::addSuccessExchange(dev::brc::ex::result_order const &_order) {
    Account *_orderAccount = account(dev::ExdbSystemAddress);
    if (!_orderAccount) {
        createAccount(dev::ExdbSystemAddress, {0});
        _orderAccount = account(dev::ExdbSystemAddress);
    }
    dev::brc::ex::ExResultOrder _oldOrder = _orderAccount->getSuccessOrder();
    _orderAccount->addSuccessExchangeOrder(_order);
    m_changeLog.emplace_back(Change::SuccessOrder, dev::ExdbSystemAddress, _oldOrder);
}

void dev::brc::State::setSuccessExchange(dev::brc::ex::ExResultOrder const &_exresultOrder) {
    Account *_orderAccount = account(dev::ExdbSystemAddress);
    if (!_orderAccount) {
        createAccount(dev::ExdbSystemAddress, {0});
        _orderAccount = account(dev::ExdbSystemAddress);
    }

    dev::brc::ex::ExResultOrder _oldOrder = _orderAccount->getSuccessOrder();
    _orderAccount->setSuccessOrder(_exresultOrder);
    m_changeLog.emplace_back(Change::SuccessOrder, dev::ExdbSystemAddress, _oldOrder);
}


void dev::brc::State::tryChangeMiner(const dev::brc::BlockHeader &curr_header, ChainParams const &params) {
    if (params.chainID != 11)
        return;
    auto change_ret = config::getChainMiner(curr_header.number());
    if (!change_ret.first)
        return;
    changeMinerMigrationData(Address(change_ret.second.before_addr), Address(change_ret.second.new_addr), params);
}

void dev::brc::State::changeMinerAddVote(BlockHeader const &_header) {
//     if(_header.number() == 4756444){
//         std::vector<std::tuple<std::string, std::string, std::string>> _changeVote = changeVote::getChangeMinerAddVote();
//         for (auto const& it : _changeVote) {
//             Account *_a = account(Address(std::get<0>(it)));
//             Account *_pollA = account(Address(std::get<1>(it)));
//             if(!_a){
//                 createAccount(Address(std::get<0>(it)), {0});
//                 _a = account(Address(std::get<0>(it)));
//             }
//             if(!_pollA){
//                 createAccount(Address(std::get<1>(it)), {0});
//                 _pollA = account(Address(std::get<1>(it)));
//             }

//             VoteSnapshot a_snashot = _a->vote_snashot();
//             if (a_snashot.m_voteDataHistory.empty()) {
//                 _a->addVote(std::pair<Address, u256>(Address(std::get<1>(it)), u256(std::get<2>(it))));

//             } else{
//                 //VoteSnapshot a_snashot = _a->vote_snashot();
//                 for (auto & d : a_snashot.m_voteDataHistory){
//                     if(d.second.count(Address(std::get<1>(it)))){
//                         d.second[Address(std::get<1>(it))] += u256(std::get<2>(it));
//                     } else{
//                         d.second[Address(std::get<1>(it))] = u256(std::get<2>(it));
//                     }
//                 }
//                 _a->set_vote_snapshot(a_snashot);

//             }

//             VoteSnapshot a_snashot_A = _pollA->vote_snashot();
//             if (a_snashot_A.m_pollNumHistory.empty()) {
//                 _pollA->addPoll(u256(std::get<2>(it)));

//             } else{
//                 if(a_snashot_A.m_pollNumHistory.count(1)){
//                     a_snashot_A.m_pollNumHistory[1] += u256(std::get<2>(it));
//                 }
//                 else {
//                     a_snashot_A.m_pollNumHistory[1] = u256(std::get<2>(it));
//                 }
//                 _pollA->set_vote_snapshot(a_snashot_A);
//             }

//         }


//         Account *sysVar = account(SysVarlitorAddress);
//         std::vector<PollData> _sysVar = sysVar->vote_data();
//         Account *sysCan = account(SysCanlitorAddress);
//         std::vector<PollData> _sysCan = sysCan->vote_data();
//         for (auto &sysVarIt : _sysVar) {
//             Account *pa = account(sysVarIt.m_addr);
// //            CFEE_LOG << pa->poll();
//             auto  snapshot = pa->vote_snashot();
//             if(snapshot.m_pollNumHistory.empty()) {
//                 sysVarIt.m_poll = pa->poll();
//                 sysVar->set_system_poll({sysVarIt.m_addr, pa->poll(), 0});
//             }
//             else {
//                 sysVar->set_system_poll({sysVarIt.m_addr, snapshot.m_pollNumHistory[1], 0});
//             }
//         }

//         for (auto &sysCanIt : _sysCan) {
//             Account *pa = account(sysCanIt.m_addr);
//             if(!pa){
//                 createAccount(sysCanIt.m_addr, {0});
//                 pa = account(sysCanIt.m_addr);
//             }
// //            CFEE_LOG << pa->poll();
//             auto  snapshot = pa->vote_snashot();
//             if(snapshot.m_pollNumHistory.empty()) {
//                 sysCanIt.m_poll = pa->poll();
//                 sysCan->set_system_poll({sysCanIt.m_addr, pa->poll(), 0});
//             } else{
//                 sysVar->set_system_poll({sysCanIt.m_addr, snapshot.m_pollNumHistory[1], 0});
//             }
//         }

//         Account *_pdSystem = account(PdSystemAddress);
//         CouplingSystemfee _pdfee = _pdSystem->getFeeSnapshot();
//         for (auto &_pdit : _pdfee.m_sorted_creaters) {
//             std::vector<PollData> _poll = vote_data(SysVarlitorAddress);
//             _pdit.second = _poll;
//         }
// //        CFEE_LOG << " LINSHI:  " << _pdfee;
//         _pdSystem->setCouplingSystemFeeSnapshot(_pdfee);
// //        CFEE_LOG <<_pdSystem->getFeeSnapshot();

//         Account *_minerSystem = account(SysMinerSnapshotAddress);
//         CouplingSystemfee _minerfee = _minerSystem->getFeeSnapshot();
//         for (auto &_minerit : _minerfee.m_sorted_creaters) {
//             std::vector<PollData> _poll = vote_data(SysVarlitorAddress);
//             std::vector<PollData> _cpoll = vote_data(SysCanlitorAddress);
//             _poll.insert(_poll.end(), _cpoll.begin(), _cpoll.end());
//             _minerit.second = _poll;
//         }
//         _minerSystem->setCouplingSystemFeeSnapshot(_minerfee);
//     }
}

void dev::brc::State::changeVoteData(BlockHeader const &_header) {
    if (config::changeVoteHeight() == _header.number()) {
        std::vector<std::tuple<std::string, std::string, std::string>> _changeVote = changeVote::getChangeVote();
        for (auto it : _changeVote) {
            Account *_a = account(Address(std::get<0>(it)));
            Account *_pollA = account(Address(std::get<1>(it)));
            _a->addVote(std::pair<Address, u256>(Address(std::get<1>(it)), u256(std::get<2>(it))));
            _pollA->addPoll(u256(std::get<2>(it)));
        }


        Account *sysVar = account(SysVarlitorAddress);
        std::vector<PollData> _sysVar = sysVar->vote_data();
        Account *sysCan = account(SysCanlitorAddress);
        std::vector<PollData> _sysCan = sysCan->vote_data();
        for (auto &sysVarIt : _sysVar) {
            Account *pa = account(sysVarIt.m_addr);
//            CFEE_LOG << pa->poll();
            sysVarIt.m_poll = pa->poll();
            sysVar->set_system_poll({sysVarIt.m_addr, pa->poll(), 0});
        }

        for (auto &sysCanIt : _sysCan) {
            Account *pa = account(sysCanIt.m_addr);
//            CFEE_LOG << pa->poll();
            sysCanIt.m_poll = pa->poll();
            sysCan->set_system_poll({sysCanIt.m_addr, pa->poll(), 0});
        }

        Account *_pdSystem = account(PdSystemAddress);
        CouplingSystemfee _pdfee = _pdSystem->getFeeSnapshot();
        for (auto &_pdit : _pdfee.m_sorted_creaters) {
            std::vector<PollData> _poll = vote_data(SysVarlitorAddress);
            _pdit.second = _poll;
        }
//        CFEE_LOG << " LINSHI:  " << _pdfee;
        _pdSystem->setCouplingSystemFeeSnapshot(_pdfee);
//        CFEE_LOG <<_pdSystem->getFeeSnapshot();

        Account *_minerSystem = account(SysMinerSnapshotAddress);
        CouplingSystemfee _minerfee = _minerSystem->getFeeSnapshot();
        for (auto &_minerit : _minerfee.m_sorted_creaters) {
            std::vector<PollData> _poll = vote_data(SysVarlitorAddress);
            std::vector<PollData> _cpoll = vote_data(SysCanlitorAddress);
            _poll.insert(_poll.end(), _cpoll.begin(), _cpoll.end());
            _minerit.second = _poll;
        }
        _minerSystem->setCouplingSystemFeeSnapshot(_minerfee);
//        CFEE_LOG << " LINSHI:  " << _minerfee;
//        CFEE_LOG <<_minerSystem->getFeeSnapshot();
    }
    return;
}

void dev::brc::State::changeTestMiner(BlockHeader const &_header) {
    if(_header.number() == 4755702){
        /// change testMiner
        //b172c802467c5507f2919ec417873a7fae167557 5JowCLg2pCFzXWmHoeNYCHSjRJGZhkgCx8PDjsfJ6Njp
        //e3688750ecc7fa4a342c2127fe153b4306559b20 8kigMjSsDZ1Y36mMzrgcUxmzNL5YGcsP1qPJ5pVZFt1f
        auto a = account(SysCanlitorAddress);
        if(a){
            std::vector<PollData> standby_miners = a->vote_data();
            standby_miners[0].m_addr = Address("b172c802467c5507f2919ec417873a7fae167557");
            standby_miners[4].m_addr = Address("e3688750ecc7fa4a342c2127fe153b4306559b20");
            a->set_vote_data(standby_miners);
        }
    }
}

void dev::brc::State::changeMinerMigrationData(const dev::Address &before_addr, const dev::Address &new_addr,
                                               ChainParams const &params) {
    Account *old_a = account(before_addr);
    if (!old_a) {
        // throw
        BOOST_THROW_EXCEPTION(InvalidAutor() << errinfo_comment(
                std::string("changeMiner: not have this author:" + dev::toString(before_addr))));
    }
    Account *new_a = account(new_addr);
    if (!new_a) {
        createAccount(new_addr, {0});
        new_a = account(new_addr);
    } else {
        kill(new_addr);
    }
    /// account's base data
    new_a->copyByAccount(*old_a);
    new_a->changeMinerUpdateData(before_addr, new_addr);
    old_a->kill();

    /// system Accounts
    std::vector<Address> sys_accounts;
    sys_accounts.emplace_back(systemAddress);
    sys_accounts.emplace_back(PdSystemAddress);
    sys_accounts.emplace_back(SysBlockCreateRecordAddress);
    sys_accounts.emplace_back(SysElectorAddress);
    sys_accounts.emplace_back(SysVarlitorAddress);
    sys_accounts.emplace_back(SysCanlitorAddress);
    sys_accounts.emplace_back(SysMinerSnapshotAddress);
    for (auto const &v : sys_accounts) {
        if (auto sys_a = account(v)) {
            if (sys_a->isChangeMinerUpdateData(before_addr, new_addr)) {
                sys_a->changeMinerUpdateData(before_addr, new_addr);
            }
        }
    }
    /// loop all
    /// other account vote data in genesis
    for (auto const &v: params.genesisState) {
        if (!v.second.isChangeMinerUpdateData(before_addr, new_addr))
            continue;
        Account *temp_a = account(v.first);
        if (!temp_a)
            continue;
        temp_a->changeMinerUpdateData(before_addr, new_addr);
    }
}

void dev::brc::State::transferOldExData(BlockHeader const &_header){
    //TODO copy old ex_data to new_ex
    if(_header.number() != config::changeOldExDataHeight())
        return;
    Account *_orderAccount = account(dev::ExdbSystemAddress);
    if (!_orderAccount) {
        BOOST_THROW_EXCEPTION(ExdbChangeFailed() << errinfo_comment("transferOldExData failed"));
    }

    Account *_buyAccount = account(dev::BuyExchangeAddress);
    Account *_sellAccount = account(dev::SellExchangeAddress);
//    if(!_buyAccount){
//        createAccount(dev::ExdbSystemAddress, {0});
//        _buyAccount = account(dev::BuyExchangeAddress);
//    }
//    if(!_sellAccount){
//        createAccount(dev::ExdbSystemAddress, {0});
//        _sellAccount = account(dev::SellExchangeAddress);
//    }
    const auto &index= _orderAccount->getExOrder().get<ex_by_trx_id>();
    auto begin = index.begin();
    while (begin != index.end() ) {
        newAddExchangeOrder(begin->sender,*begin);
        begin++;
//        exchangeValue eo;
//        eo.m_orderId = begin->trxid;
//        eo.m_from = begin->sender;
//        eo.m_pendingorderNum = begin->source_amount;
//        eo.m_pendingordertokenNum = begin->token_amount;
//        eo.m_pendingorderPrice = begin->source_amount;
//        eo.m_createTime = begin->create_time;
//        eo.m_pendingorderType = begin->type;
//        eo.m_pendingorderTokenType = begin->token_type;
//        eo.m_pendingorderBuyType = begin->buy_type;
//        if(eo.m_pendingorderType == ex::order_type::buy){
//
//        }
//        else if(eo.m_pendingorderType == ex::order_type::sell){
//
//        } else{
//            BOOST_THROW_EXCEPTION(ExdbChangeFailed() << errinfo_comment("transferOldExData failed"));
//        }
    }
    _orderAccount->clearExOrderMulti();
}

void dev::brc::State::testBplus(const std::vector<std::shared_ptr<transationTool::operation>> &_ops, int64_t const& _blockNum)
{
    Account *_account = account(dev::TestbplusAddress);
    if(_account == NULL)
    {
        createAccount(dev::TestbplusAddress, {0});
        _account = account(dev::TestbplusAddress);
    }
    for(auto it : _ops)
    {
        std::shared_ptr<transationTool::testBplus_operation> _op = std::dynamic_pointer_cast<transationTool::testBplus_operation>(it);
        std::unordered_map<h256, bytes> _oldmap = _account->storageByteOverlay();
        std::vector<h256> _oldv = _account->getDeleteByte();
        if(_op->testType == transationTool::testBplusType::BplusAdd)
        {   
             _account->testBplusAdd(_op->testKey, _op->testValue, _blockNum, _op->testId, m_db);
             m_changeLog.emplace_back(dev::TestbplusAddress, _oldmap);
        }else if(_op->testType == transationTool::testBplusType::BplusChange)
        {
            _account->testBplusAdd(_op->testKey, _op->testValue, _blockNum, _op->testId, m_db);
             m_changeLog.emplace_back(dev::TestbplusAddress,_oldmap);
        }else if(_op->testType == transationTool::testBplusType::BplusDelete)
        {
             _account->testBplusDelete(_op->testKey, m_db, _blockNum, _op->testId);
             m_changeLog.emplace_back(dev::TestbplusAddress,_oldv);
        }
    }
}

Json::Value dev::brc::State::testBplusGet(uint32_t const& _id, int64_t const& _blockNum)
{
    Account *_a = account(dev::TestbplusAddress);
    if(!_a)
    {
        cerror << "error";
        return Json::Value();
    }
    dev::brc::transationTool::testDetails _testDetails = _a->testBplusGet(_blockNum, _id, m_db);
    cerror << "testDetail : " << _testDetails;
    Json::Value _ret;
    _ret["firstData"] = _testDetails.firstData;
    _ret["secondData"] = _testDetails.secondData;
    return _ret;
}

dev::brc::ex::ExResultOrder const &dev::brc::State::getSuccessExchange() {
    Account *_SuccessAccount = account(dev::ExdbSystemAddress);
    if (!_SuccessAccount) {
        BOOST_THROW_EXCEPTION(SuccessExChangeFailed() << errinfo_comment("getSuccessExchange failed"));
    }

    return _SuccessAccount->getSuccessOrder();
}

std::ostream &dev::brc::operator<<(std::ostream &_out, State const &_s) {
    _out << "--- " << _s.rootHash() << std::endl;
    std::set<Address> d;
    std::set<Address> dtr;
    auto trie = SecureTrieDB<Address, OverlayDB>(const_cast<OverlayDB *>(&_s.m_db), _s.rootHash());
    for (auto i : trie)
        d.insert(i.first), dtr.insert(i.first);
    for (auto i : _s.m_cache)
        d.insert(i.first);

    for (auto i : d) {
        auto it = _s.m_cache.find(i);
        Account *cache = it != _s.m_cache.end() ? &it->second : nullptr;
        string rlpString = dtr.count(i) ? trie.at(i) : "";
        RLP r(rlpString);
        assert(cache || r);

        if (cache && !cache->isAlive())
            _out << "XXX  " << i << std::endl;
        else {
            string lead = (cache ? r ? " *   " : " +   " : "     ");
            if (cache && r && cache->nonce() == r[0].toInt<u256>() &&
                cache->balance() == r[1].toInt<u256>())
                lead = " .   ";

            stringstream contout;

            if ((cache && cache->codeHash() == EmptySHA3) ||
                (!cache && r && (h256) r[3] != EmptySHA3)) {
                std::map<u256, u256> mem;
                std::set<u256> back;
                std::set<u256> delta;
                std::set<u256> cached;
                if (r) {
                    SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB *>(&_s.m_db),
                                                        r[2].toHash<h256>());  // promise we won't alter the overlay! :)
                    for (auto const &j : memdb)
                        mem[j.first] = RLP(j.second).toInt<u256>(), back.insert(j.first);
                }
                if (cache)
                    for (auto const &j : cache->storageOverlay()) {
                        if ((!mem.count(j.first) && j.second) ||
                            (mem.count(j.first) && mem.at(j.first) != j.second))
                            mem[j.first] = j.second, delta.insert(j.first);
                        else if (j.second)
                            cached.insert(j.first);
                    }
                if (!delta.empty())
                    lead = (lead == " .   ") ? "*.*  " : "***  ";

                contout << " @:";
                if (!delta.empty())
                    contout << "???";
                else
                    contout << r[2].toHash<h256>();
                if (cache && cache->hasNewCode())
                    contout << " $" << toHex(cache->code());
                else
                    contout << " $" << (cache ? cache->codeHash() : r[3].toHash<h256>());

                for (auto const &j : mem)
                    if (j.second)
                        contout << std::endl
                                << (delta.count(j.first) ?
                                    back.count(j.first) ? " *     " : " +     " :
                                    cached.count(j.first) ? " .     " : "       ")
                                << std::hex << nouppercase << std::setw(64) << j.first << ": "
                                << std::setw(0) << j.second;
                    else
                        contout << std::endl
                                << "XXX    " << std::hex << nouppercase << std::setw(64) << j.first
                                << "";
            } else
                contout << " [SIMPLE]";
            _out << lead << i << ": " << std::dec << (cache ? cache->nonce() : r[0].toInt<u256>())
                 << " #:" << (cache ? cache->balance() : r[1].toInt<u256>()) << contout.str()
                 << std::endl;
        }
    }
    return _out;
}

State &dev::brc::createIntermediateState(
        State &o_s, Block const &_block, unsigned _txIndex, BlockChain const &_bc) {
    o_s = _block.state();
    u256 const rootHash = _block.stateRootBeforeTx(_txIndex);
    if (rootHash)
        o_s.setRoot(rootHash);
    else {
        o_s.setRoot(_block.stateRootBeforeTx(0));
        o_s.executeBlockTransactions(_block, _txIndex, _bc.lastBlockHashes(), *_bc.sealEngine());
    }
    return o_s;
}

template<class DB>
AddressHash
dev::brc::commit(AccountMap const &_cache, SecureTrieDB<Address, DB> &_state, int64_t _block_number /*= 0*/) {
    AddressHash ret;
    for (auto const &i : _cache)
        if (i.second.isDirty()) {

            if (!i.second.isAlive())
                _state.remove(i.first);
            else {
                RLPStream s;
                if (_block_number >= config::newChangeHeight()) {
                    s.appendList(20);
                    if(_block_number >= config::changeExchange()){
                        s.appendList(21);
                    }
                }
                else{
                    s.appendList(19);
                }

                s << i.second.nonce() << i.second.balance();
                if (i.second.storageOverlay().empty()) {
                    assert(i.second.baseRoot());
                    s.append(i.second.baseRoot());
                } else {
                    SecureTrieDB<h256, DB> storageDB(_state.db(), i.second.baseRoot());
                    for (auto const &j : i.second.storageOverlay())
                        if (j.second)
                            storageDB.insert(j.first, rlp(j.second));
                        else
                            storageDB.remove(j.first);
                    assert(storageDB.root());
                    s.append(storageDB.root());
                }

                if (i.second.hasNewCode()) {
                    h256 ch = i.second.codeHash();
                    // Store the size of the code
                    CodeSizeCache::instance().store(ch, i.second.code().size());
                    _state.db()->insert(ch, &i.second.code());
                    s << ch;
                } else
                    s << i.second.codeHash();
                s << i.second.ballot();
                s << i.second.poll();
                {
                    std::vector<bytes> bs;
                    for (auto const &val: i.second.vote_data()) {
                        RLPStream _s;
                        val.streamRLP(_s);
                        bs.push_back(_s.out());
                    }
                    s.appendVector<bytes>(bs);
                }
                s << i.second.BRC();
                s << i.second.FBRC();
                s << i.second.FBalance();
                {
                    RLPStream _rlp;
                    size_t _num = i.second.blockReward().size();
                    _rlp.appendList(_num + 1);
                    _rlp << _num;
                    for (auto it : i.second.blockReward()) {
                        _rlp.append<u256, u256>(std::make_pair(it.first, it.second));
                    }
                    s << _rlp.out();
                }

                s << i.second.willChangeList();
                s << i.second.CookieIncome();
                {
                    RLPStream _s;
                    i.second.vote_snashot().streamRLP(_s);
                    s << _s.out();
                }
                {
                    RLPStream _feeRlp;
                    i.second.getFeeSnapshot().streamRLP(_feeRlp);
                    s << _feeRlp.out();
                }
                s << i.second.block_record().streamRLP();

                s << i.second.get_received_cookies().streamRLP();

                {
                    s << i.second.getStreamRLPResultOrder();
                    s << i.second.getStreamRLPExOrder();
                }

                {
                    /// fork about newChangeMiner
                    if(_block_number >= config::newChangeHeight()){
                        s << i.second.getRLPStreamChangeMiner();
                        //cwarn << " insert rlp:" << dev::toString(i.second.getRLPStreamChangeMiner());
                    }
                }
                
                //Add a new state field
                {
                    if(i.second.storageByteOverlay().empty() && i.second.getExchangeDelete().empty())
                    {
                        assert(i.second.baseByteRoot());
                        s << i.second.baseByteRoot();
                    }else{
                        SecureTrieDB<h256, DB> storageByteDB(_state.db(), i.second.baseByteRoot());
                        for(auto const& val : i.second.storageByteOverlay())
                        {
                            if(!val.second.empty())
                            {
                                storageByteDB.insert(val.first, val.second);
                            }else{
                                storageByteDB.remove(val.first);
                            }
                        }
                        for(auto const& keyVal : i.second.getExchangeDelete())
                        {
                            storageByteDB.remove(keyVal);
                        }
                        assert(storageByteDB.root());
                        s << storageByteDB.root();
                    }
                }
                _state.insert(i.first, &s.out());
            }
            ret.insert(i.first);
        }
    return ret;
}

template AddressHash dev::brc::commit<OverlayDB>(
        AccountMap const &_cache, SecureTrieDB<Address, OverlayDB> &_state, int64_t _block_number = 0);

template AddressHash dev::brc::commit<StateCacheDB>(
        AccountMap const &_cache, SecureTrieDB<Address, StateCacheDB> &_state, int64_t _block_number = 0);
