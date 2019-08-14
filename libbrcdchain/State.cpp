#include "State.h"

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

using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace dev::brc::ex;

namespace fs = boost::filesystem;

#define BRCNUM 1000
#define COOKIENUM 100000000000


State::State(u256 const& _accountStartNonce, OverlayDB const& _db, ex::exchange_plugin const& _exdb,
    BaseState _bs)
  : m_db(_db), m_exdb(_exdb), m_state(&m_db), m_accountStartNonce(_accountStartNonce)
{
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
          m_accountStartNonce(_s.m_accountStartNonce) {}

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
    if (_we == WithExisting::Kill) {
        clog(VerbosityDebug, "exdb") << "Killing state database (WithExisting::Kill).";
        fs::remove_all(_path);
    }
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
    brc::commit(_map, m_state, timestamp());
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
    for(auto val : state[6].toVector<bytes>()){
        PollData p_data;
        p_data.populate(val);
        _vote.push_back(p_data);
    }

	const bytes _bBlockReward = state[10].toBytes();
	RLP _rlpBlockReward(_bBlockReward);
	size_t num = _rlpBlockReward[0].toInt<size_t>();
	std::vector<std::pair<u256, u256>> _blockReward;
	for (size_t k = 1; k <= num; k++)
	{
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

	u256 _cookieNum  = state[12].toInt<u256>();
	i.first->second.setCookieIncome(_cookieNum);

    const bytes  vote_sna_b = state[13].convert<bytes>(RLP::LaissezFaire);
    i.first->second.init_vote_snapshot(vote_sna_b);

    const bytes _feeSnapshotBytes = state[14].convert<bytes>(RLP::LaissezFaire);
    i.first->second.initCoupingSystemFee(_feeSnapshotBytes);

    const bytes record_b = state[15].convert<bytes>(RLP::LaissezFaire);
    i.first->second.init_block_record(record_b);
    const bytes received_cookies = state[16].convert<bytes>(RLP::LaissezFaire);
    i.first->second.init_received_cookies(received_cookies);
    m_unchangedCacheEntries.push_back(_addr);

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
    m_touched += dev::brc::commit(m_cache, m_state, timestamp());
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
    if (!a || a->BRC() < _value){
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
    if (!a || a->FBRC() < _value){
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
void State::pendingOrder(Address const& _addr, u256 _pendingOrderNum, u256 _pendingOrderPrice,
                         h256 _pendingOrderHash, ex::order_type _pendingOrderType, ex::order_token_type _pendingOrderTokenType,
                         ex::order_buy_type _pendingOrderBuyType, int64_t _nowTime) {
    // add
    //freezeAmount(_addr, _pendingOrderNum, _pendingOrderPrice, _pendingOrderType,
      //           _pendingOrderTokenType, _pendingOrderBuyType);

	u256 _totalSum;
	if (_pendingOrderBuyType == order_buy_type::only_price)
	{
		_totalSum = _pendingOrderNum * _pendingOrderPrice / PRICEPRECISION;
	}
	else {
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

	if (_pendingOrderBuyType == order_buy_type::only_price)
	{
		if (_pendingOrderType == order_type::buy && _pendingOrderTokenType == order_token_type::BRC)
        {
            if(CombinationNum < _pendingOrderNum) {
                subBRC(_addr, _totalSum - CombinationTotalAmount);
                addFBRC(_addr, _totalSum - CombinationTotalAmount);
            }
		}
		else if (_pendingOrderType == order_type::buy && _pendingOrderTokenType == order_token_type::FUEL)
		{
            if(CombinationNum < _pendingOrderNum)
            {
                subBalance(_addr, _totalSum - CombinationTotalAmount);
                addFBalance(_addr, _totalSum - CombinationTotalAmount);
            }

		}
		else if (_pendingOrderType == order_type::sell && _pendingOrderTokenType == order_token_type::BRC)
		{
			subBRC(_addr, _pendingOrderNum - CombinationNum);
			addFBRC(_addr, _pendingOrderNum - CombinationNum);
		}
		else if (_pendingOrderType == order_type::sell && _pendingOrderTokenType == order_token_type::FUEL)
		{
			subBalance(_addr, _pendingOrderNum - CombinationNum);
			addFBalance(_addr, _pendingOrderNum - CombinationNum);
		}
	}
}

void dev::brc::State::changeMiner(std::vector<std::shared_ptr<transationTool::operation>> const& _ops){
	std::shared_ptr<transationTool::changeMiner_operation> pen = std::dynamic_pointer_cast<transationTool::changeMiner_operation>(_ops[0]);
    Account *a = account(SysVarlitorAddress);
    a->insertMiner(pen->m_before, pen->m_after, pen->m_blockNumber);
        
	
}

Account* dev::brc::State::getSysAccount(){
    return account(SysVarlitorAddress);
}

void dev::brc::State::pendingOrders(Address const& _addr, int64_t _nowTime, h256 _pendingOrderHash, std::vector<std::shared_ptr<transationTool::operation>> const& _ops){
	std::vector<order> _v ;
	std::vector<result_order> _result_v;

	bigint total_free_brc = 0;
	bigint total_free_balance = 0;

	for(auto const& val : _ops){
		std::shared_ptr<transationTool::pendingorder_opearaion> pen = std::dynamic_pointer_cast<transationTool::pendingorder_opearaion>(val);
		if(!pen){
			cerror << "pendingOrders  dynamic type field!";
			BOOST_THROW_EXCEPTION(InvalidDynamic());
		}

		std::pair<u256, u256> _pair;
		if(pen->m_Pendingorder_buy_type == order_buy_type::all_price && pen->m_Pendingorder_type == order_type::buy) {
            _pair = {pen->m_Pendingorder_price * PRICEPRECISION, pen->m_Pendingorder_num};
        }else{
            _pair =  {pen->m_Pendingorder_price, pen->m_Pendingorder_num};
		}

		order _order = { _pendingOrderHash, _addr, (order_buy_type)pen->m_Pendingorder_buy_type,
						(order_token_type)pen->m_Pendingorder_Token_type, (order_type)pen->m_Pendingorder_type, _pair, _nowTime };
		_v.push_back(_order);

		if(pen->m_Pendingorder_buy_type == order_buy_type::only_price){
		    if(pen->m_Pendingorder_type == order_type::buy && pen->m_Pendingorder_Token_type == order_token_type::FUEL)
                total_free_brc += pen->m_Pendingorder_num * pen->m_Pendingorder_price / PRICEPRECISION;
			else if(pen->m_Pendingorder_type == order_type::sell && pen->m_Pendingorder_Token_type == order_token_type::FUEL)
				total_free_balance += pen->m_Pendingorder_num;
		}
	}
	try{
		_result_v = m_exdb.insert_operation(_v, false, true);
	}
	catch(const boost::exception &e){
		cerror << "this pendingOrder is error :" << diagnostic_information_what(e);
		BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
	}

	std::set<order_type> _set;
	for(uint32_t i = 0; i < _result_v.size(); ++i){
		result_order _result_order = _result_v[i];

		pendingOrderTransfer(_result_order.sender, _result_order.acceptor, _result_order.amount,
							 _result_order.price, _result_order.type, _result_order.token_type, _result_order.buy_type);

		if(_result_order.buy_type == order_buy_type::only_price){
		    if(_result_order.type == order_type::buy && _result_order.token_type == order_token_type::FUEL)
				total_free_brc -= _result_order.amount * _result_order.old_price / PRICEPRECISION;
			else if(_result_order.type == order_type::sell && _result_order.token_type == order_token_type::FUEL)
				total_free_balance -= _result_order.amount;
		}

		_set.emplace(_result_order.type);
	}
	if(total_free_balance < 0 || total_free_brc < 0){
		cerror << " this pindingOrder's free_balance or free_brc is error... balance:" << total_free_balance << " brc:" << total_free_brc;
		BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
	}
	if(total_free_balance > 0){
		subBalance(_addr, (u256)total_free_balance);
		addFBalance(_addr, (u256)total_free_balance);
	}
	if(total_free_brc){
		subBRC(_addr, (u256)total_free_brc);
		addFBRC(_addr, (u256)total_free_brc);
	}

	if(_set.size() > 0)
    {
	    systemAutoPendingOrder(_set, _nowTime);
    }
}

void State::systemAutoPendingOrder(std::set<order_type> const& _set, int64_t _nowTime)
{
    std::vector<result_order> _result_v;
    std::set<order_type> _autoSet;
    std::vector<order> _v;
    u256 _needBrc = 0;
    u256 _needCookie = 0;

    for(auto it : _set)
    {
        if(it == order_type::buy)
        {
            u256 _num = BRC(systemAddress) * PRICEPRECISION / BUYCOOKIE  / 10000 * 10000 ;
            _needBrc = _num * u256(BUYCOOKIE) / PRICEPRECISION;
            std::pair<u256, u256> _pair = {u256(BUYCOOKIE), _num};
            order _order = {h256(1), systemAddress, order_buy_type::only_price, order_token_type::FUEL, order_type::buy, _pair, _nowTime};
            _v.push_back(_order);
        }else if(it == order_type::sell)
        {
            u256 _num = balance(systemAddress);
            _needCookie = _num;
            std::pair<u256, u256> _pair = {u256(SELLCOOKIE), _num};
            order _order = {h256(1), systemAddress, order_buy_type::only_price, order_token_type::FUEL, order_type::sell, _pair, _nowTime};
            _v.push_back(_order);
        }
    }

    try{
        _result_v = m_exdb.insert_operation(_v, false, true);
    }
    catch(const boost::exception &e){
        cerror << "this pendingOrder is error :" << diagnostic_information_what(e);
        BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
    }

    for(auto _result : _result_v)
    {
        pendingOrderTransfer(_result.sender, _result.acceptor, _result.amount,
                             _result.price, _result.type, _result.token_type, _result.buy_type);

        if(_result.buy_type == order_buy_type::only_price){
            if(_result.type == order_type::buy && _result.token_type == order_token_type::FUEL)
                _needBrc -= _result.amount * _result.old_price / PRICEPRECISION;
            else if(_result.type == order_type::sell && _result.token_type == order_token_type::FUEL)
                _needCookie -= _result.amount;
        }
        _autoSet.emplace(_result.type);
    }
    if(_needBrc < 0 || _needCookie < 0){
        cerror << " this pindingOrder's free_balance or free_brc is error... balance:" << _needCookie << " brc:" << _needBrc;
        BOOST_THROW_EXCEPTION(pendingorderAllPriceFiled());
    }
    if(_needCookie > 0){
        subBalance(systemAddress, _needCookie);
        addFBalance(systemAddress, _needCookie);
    }
    if(_needBrc){
        subBRC(systemAddress, _needBrc);
        addFBRC(systemAddress, _needBrc);
    }
    if(_autoSet.size() > 0)
    {
        systemAutoPendingOrder(_autoSet, _nowTime);
    }
}

void State::pendingOrderTransfer(Address const& _from, Address const& _to, u256 _toPendingOrderNum,
                                 u256 _toPendingOrderPrice, ex::order_type _pendingOrderType, ex::order_token_type _pendingOrderTokenType,
                                 ex::order_buy_type _pendingOrderBuyTypes) {

    u256 _fromFee = 0;//(_toPendingOrderNum * _toPendingOrderPrice / PRICEPRECISION) / MATCHINGFEERATIO;
    u256 _toFee = 0;//_toPendingOrderNum / MATCHINGFEERATIO;

    if (_pendingOrderType == order_type::buy &&
               _pendingOrderTokenType == order_token_type::FUEL &&
            (_pendingOrderBuyTypes == order_buy_type::only_price || _pendingOrderBuyTypes == order_buy_type::all_price)) {
        if(_from != dev::systemAddress)
        {
            _fromFee = _toPendingOrderNum / MATCHINGFEERATIO;
        }
        if(_to != dev::systemAddress)
        {
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
        if(_from != dev::systemAddress)
        {
            _fromFee = (_toPendingOrderNum * _toPendingOrderPrice / PRICEPRECISION) / MATCHINGFEERATIO;
        }
        if(_to != dev::systemAddress)
        {
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

void State::freezeAmount(Address const& _addr, u256 _pendingOrderNum, u256 _pendingOrderPrice,
                         ex::order_type _pendingOrderType, ex::order_token_type _pendingOrderTokenType, ex::order_buy_type _pendingOrderBuyType) {
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

Json::Value State::queryExchangeReward(Address const& _address) {
    Json::Value res;
    res["queryExchangeReward"] = "";
    return res;
}

Json::Value State::queryBlcokReward(Address const& _address, unsigned _blockNum) {
    auto a = account(_address);
    ReceivedCookies _receivedCookies = a->get_received_cookies();
    CFEE_LOG << "before : " << _receivedCookies << endl;
    ReceivedCookies _oldreceivedCookies = a->get_received_cookies();
    VoteSnapshot _votesnapshot = a->vote_snashot();
    std::pair<u256, Votingstage> _pair = config::getVotingCycle(_blockNum);
    u256 _numberofrounds = config::getvoteRound(_receivedCookies.m_numberofRound);
    u256 _cookieFee = 0;
    u256 _isMainNodeFee = 0;
    std::map<u256, std::map<Address, u256>>::const_iterator _voteDataIt = _votesnapshot.m_voteDataHistory.find(_numberofrounds - 1);


    // If you receive the account, you will receive the income from the block node account.

    //Receive an account to receive voting dividends
    for(; _voteDataIt != _votesnapshot.m_voteDataHistory.end(); _voteDataIt++)
    {
        std::map<u256, u256>::const_iterator _pollDataIt = _votesnapshot.m_pollNumHistory.find(_voteDataIt->first);
        auto _pollNum = _pollDataIt->second;
        if(_pollNum > 0)
        {
            u256 _pollFee = 0;
            if(_pollDataIt->first + 1 < _pair.first)
            {
                if(_votesnapshot.m_blockSummaryHistory.count(_pollDataIt->first + 1))
                {
                    _pollFee = _votesnapshot.m_blockSummaryHistory.find(_pollDataIt->first + 1)->second;
                }
            }else{
                _pollFee = a->CookieIncome();
            }
            CFEE_LOG << "_pollfee:" << _pollFee << endl;
            _isMainNodeFee += _pollFee - (_pollFee / 2 / _pollNum * _pollNum);
            CFEE_LOG << "_cookieFee:" << _cookieFee << endl;
            CFEE_LOG << _receivedCookies;
        }
        for(auto _voteIt : _voteDataIt->second)
        {
            auto _pollAddr = account(_voteIt.first);
            try_new_vote_snapshot(_voteIt.first, _blockNum);
            VoteSnapshot _pollVoteSnapshot = _pollAddr->vote_snashot();
            auto _pollMap = _pollVoteSnapshot.m_pollNumHistory;
            u256 _pollNum = _pollMap.find(_voteDataIt->first)->second;
            u256 _pollCookieFee = 0;
            u256 _receivedNum = 0;
            u256 _pollFeeIncome = 0;
            u256 _numTotalFee = 0;
            if(_receivedCookies.m_received_cookies.count(_voteDataIt->first + 1))
            {
                std::map<Address, std::pair<u256, u256>> _addrReceivedCookie =_receivedCookies.m_received_cookies[_voteDataIt->first + 1];
                if(_addrReceivedCookie.count(_voteIt.first))
                {
                    _receivedNum += _addrReceivedCookie.find(_voteIt.first)->second.second;
                }
            }
            if(_voteDataIt->first + 1 < _pair.first)
            {
                if(_pollVoteSnapshot.m_blockSummaryHistory.count(_voteDataIt->first + 1))
                {
                    _pollCookieFee = _pollVoteSnapshot.m_blockSummaryHistory.find(_voteDataIt->first + 1)->second;
                }
            }else{
                _pollCookieFee = _pollAddr->CookieIncome();
            }
            CFEE_LOG << "_voteDataIt: _receivedNum: " << _receivedNum << endl;
            CFEE_LOG << "_voteDataIt :_pollCookieFee:" << _pollCookieFee << endl;
            _pollFeeIncome = _pollCookieFee / 2 / _pollNum * _voteIt.second;
            CFEE_LOG << "_voteDataIt :_cookieFee:" << _cookieFee << endl;

            if(_voteIt.first == _address)
            {
                _numTotalFee += _pollFeeIncome + _isMainNodeFee - _receivedNum;
            }else{
                _numTotalFee += _pollFeeIncome - _receivedNum;
            }
            _cookieFee += _numTotalFee;
            a->addSetreceivedCookie(_voteDataIt->first + 1, _voteIt.first, std::pair<u256, u256>(_pollCookieFee, _numTotalFee + _receivedNum));
        }
    }
    Json::Value _retVal;
    _retVal["CookieFeeIncome"] = toJS(_cookieFee);
    return  _retVal;

}

Json::Value State::pendingOrderPoolMsg(uint8_t _order_type, uint8_t _order_token_type, u256 getSize) {
    std::vector<exchange_order> _v = m_exdb.get_order_by_type(
            (order_type) _order_type, (order_token_type) _order_token_type, (uint32_t) getSize);

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

Json::Value State::pendingOrderPoolForAddrMsg(Address _a, uint32_t _getSize) {
    std::vector<exchange_order> _v = m_exdb.get_order_by_address(_a);
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

Json::Value State::successPendingOrderMsg(uint32_t _getSize) {
    std::vector<result_order> _v = m_exdb.get_result_orders_by_news(_getSize);
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
                                                 uint32_t _maxSize)
{
    std::vector<result_order> _retV = m_exdb.get_result_orders_by_address(_a, _minTime, _maxTime, _maxSize);
    Json::Value  _JsArray;

    for(auto val : _retV)
    {
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
		if(val.type == order_type::buy && val.token_type == order_token_type::FUEL){
				subFBalance(val.sender, val.price_token.second * val.price_token.first);
				addBalance(val.sender, val.price_token.second * val.price_token.first);
        } else if ( val.type == order_type::sell && val.token_type == order_token_type::FUEL) {
                subFBalance(val.sender, val.price_token.second);
                addBalance(val.sender, val.price_token.second);
        }
    }
}

void dev::brc::State::cancelPendingOrders(std::vector<std::shared_ptr<transationTool::operation>> const& _ops){
	ctrace << "cancle pendingorder";
	std::vector<ex::order> _resultV;
	std::vector<h256> _hashV;
	for(auto const& val : _ops){
		std::shared_ptr<transationTool::cancelPendingorder_operation> can_pen =std::dynamic_pointer_cast<transationTool::cancelPendingorder_operation>(val);
		if(!can_pen){ 
			cerror << "cancelPendingOrders  dynamic type field!";
			BOOST_THROW_EXCEPTION(InvalidDynamic());
		}
		_hashV.push_back({ can_pen->m_hash });
	}

	try{ 
		_resultV = m_exdb.cancel_order_by_trxid(_hashV, false);
	}
	catch(Exception &e){
		cwarn << "cancelPendingorder Error :" << e.what();
		BOOST_THROW_EXCEPTION(CancelPendingOrderFiled());
	}

	for(auto val : _resultV){
	    if(val.type == order_type::buy && val.token_type == order_token_type::FUEL){
				subFBRC(val.sender, val.price_token.second);
				addBRC(val.sender, val.price_token.second);
		}else if(val.type == order_type::sell && val.token_type == order_token_type::FUEL)
        {
            subFBalance(val.sender, val.price_token.second);
            addBalance(val.sender, val.price_token.second);
        }
	}
}

void State::addBlockReward(Address const & _addr, u256 _blockNum, u256 _rewardNum)
{
    std::pair< u256, u256> _pair = { _blockNum, _rewardNum};
	if (auto a = account(_addr))
	{
		a->addBlockRewardRecoding(_pair);
	}else{
        createAccount(_addr, {requireAccountStartNonce(), 0});
        auto _a = account(_addr);
        _a->addBlockRewardRecoding(_pair);
    }
    
    if(_rewardNum)
    {
       m_changeLog.emplace_back(_addr, std::make_pair(_blockNum, _rewardNum));
    }
}



void State::addCooikeIncomeNum(const dev::Address &_addr, const dev::u256 &_value)
{
    if(auto a = account(_addr))
    {
        a->addCooikeIncome(_value);
    }else{
        BOOST_THROW_EXCEPTION(InvalidAddressAddr());
    }

    if(_value)
    {
        m_changeLog.emplace_back(Change::CooikeIncomeNum, _addr, _value);
    }

}

void State::subCookieIncomeNum(const dev::Address &_addr, const dev::u256 &_value)
{
    if(_value == 0)
    {
        return;
    }

    auto a = account(_addr);
    if(!a || a->CookieIncome())
    {
        BOOST_THROW_EXCEPTION(NotEnoughCash() << errinfo_comment(std::string("Account does not exist or account CookieIncome is too low ")));
    }

    addCooikeIncomeNum(_addr, 0 - _value);
}

void State::setCookieIncomeNum(const dev::Address &_addr, const dev::u256 &_value)
{
    Account *a = account(_addr);
    u256 original = a ? a->CookieIncome() : 0;

    // Fall back to addBalance().
    addCooikeIncomeNum(_addr, _value - original);
}

std::unordered_map<Address, u256> State::incomeSummary(const dev::Address &_addr, uint32_t _snapshotNum)
{
    if(auto a = account(_addr))
    {
        return a->findSnapshotSummary(_snapshotNum);
    }else{
        return std::unordered_map<Address, u256>();
    }
}

void State::receivingIncome(const dev::Address &_addr, std::vector<std::shared_ptr<transationTool::operation>> const& _ops, int64_t _blockNum)
{
    try_new_vote_snapshot(_addr, _blockNum);
    for(auto _it : _ops)
    {
        std::shared_ptr<transationTool::receivingincome_operation> _op =  std::dynamic_pointer_cast<transationTool::receivingincome_operation>(_it);
        if(!_op)
        {
            BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("receivingincome_operation is error")));
        }

        ReceivingType _receType = (ReceivingType)_op->m_receivingType;
        if(_receType == ReceivingType::RBlockFeeIncome)
        {
            receivingBlockFeeIncome(_addr, _blockNum);
        }else if(_receType == ReceivingType::RPdFeeIncome)
        {
            anytime_receivingPdFeeIncome(_addr, _blockNum);
        }
    }
}

void State::receivingBlockFeeIncome(const dev::Address &_addr, int64_t _blockNum) {
    auto a = account(_addr);
    ReceivedCookies _receivedCookies = a->get_received_cookies();
    ReceivedCookies _oldreceivedCookies = a->get_received_cookies();
    VoteSnapshot _votesnapshot = a->vote_snashot();
    std::pair<u256, Votingstage> _pair = config::getVotingCycle(_blockNum);
    u256 _numberofrounds = config::getvoteRound(_receivedCookies.m_numberofRound);
    u256 _cookieFee = 0;
    u256 _isMainNodeFee = 0;
    std::map<u256, std::map<Address, u256>>::const_iterator _voteDataIt = _votesnapshot.m_voteDataHistory.find(_numberofrounds - 1);


    // If you receive the account, you will receive the income from the block node account.

    //Receive an account to receive voting dividends
    for(; _voteDataIt != _votesnapshot.m_voteDataHistory.end(); _voteDataIt++)
    {
        std::map<u256, u256>::const_iterator _pollDataIt = _votesnapshot.m_pollNumHistory.find(_voteDataIt->first);
        auto _pollNum = _pollDataIt->second;
        if(_pollNum > 0)
        {
            u256 _pollFee = 0;
            if(_pollDataIt->first + 1 < _pair.first)
            {
                if(_votesnapshot.m_blockSummaryHistory.count(_pollDataIt->first + 1))
                {
                    _pollFee = _votesnapshot.m_blockSummaryHistory.find(_pollDataIt->first + 1)->second;
                }
            }else{
                _pollFee = a->CookieIncome();
            }
            _isMainNodeFee += _pollFee - (_pollFee / 2 / _pollNum * _pollNum);
        }
        for(auto _voteIt : _voteDataIt->second)
        {
            auto _pollAddr = account(_voteIt.first);
            try_new_vote_snapshot(_voteIt.first, _blockNum);
            VoteSnapshot _pollVoteSnapshot = _pollAddr->vote_snashot();
            auto _pollMap = _pollVoteSnapshot.m_pollNumHistory;
            u256 _pollNum = _pollMap.find(_voteDataIt->first)->second;
            u256 _pollCookieFee = 0;
            u256 _receivedNum = 0;
            u256 _pollFeeIncome = 0;
            u256 _numTotalFee = 0;
            if(_receivedCookies.m_received_cookies.count(_voteDataIt->first + 1))
            {
                std::map<Address, std::pair<u256, u256>> _addrReceivedCookie =_receivedCookies.m_received_cookies[_voteDataIt->first + 1];
                if(_addrReceivedCookie.count(_voteIt.first))
                {
                    _receivedNum += _addrReceivedCookie.find(_voteIt.first)->second.second;
                }
            }
            if(_voteDataIt->first + 1 < _pair.first)
            {
                if(_pollVoteSnapshot.m_blockSummaryHistory.count(_voteDataIt->first + 1))
                {
                    _pollCookieFee = _pollVoteSnapshot.m_blockSummaryHistory.find(_voteDataIt->first + 1)->second;
                }
            }else{
                _pollCookieFee = _pollAddr->CookieIncome();
            }
            CFEE_LOG << "_voteDataIt: _receivedNum: " << _receivedNum << endl;
            CFEE_LOG << "_voteDataIt :_pollCookieFee:" << _pollCookieFee << endl;
            _pollFeeIncome = _pollCookieFee / 2 / _pollNum * _voteIt.second;
            CFEE_LOG << "_voteDataIt :_cookieFee:" << _cookieFee << endl;

            if(_voteIt.first == _addr)
            {
                _numTotalFee += _pollFeeIncome + _isMainNodeFee - _receivedNum;
            }else{
                _numTotalFee += _pollFeeIncome - _receivedNum;
            }
            _cookieFee += _numTotalFee;
            a->addSetreceivedCookie(_voteDataIt->first + 1, _voteIt.first, std::pair<u256, u256>(_pollCookieFee, _numTotalFee + _receivedNum));
        }
    }
    addBalance(_addr, _cookieFee);
    a->updateNumofround(_pair.first);
    CFEE_LOG << "new :" << a->get_received_cookies() << endl;
    m_changeLog.emplace_back(Change::ReceiveCookies, _addr, _oldreceivedCookies);
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

void State::receivingPdFeeIncome(const dev::Address &_addr, int64_t _blockNum)
{
    std::pair<u256, Votingstage> _pair = config::getVotingCycle(_blockNum);
    u256 rounds = _pair.first > 0 ? _pair.first - 1 : 0;

    auto a = account(_addr);
    auto systemAccount = account(dev::PdSystemAddress);

    u256 _numofRounds = a->getFeeNumofRounds();
    VoteSnapshot _voteSnapshot = a->vote_snashot();
    CouplingSystemfee _couplingSystemfee = systemAccount->getFeeSnapshot();
    std::map<u256, std::map<Address, u256>>::const_iterator _voteDataIt = _voteSnapshot.m_voteDataHistory.find(_numofRounds + 1);
    u256 _BrcIncome = 0;
    u256 _CookieIncome = 0;
    for(; _voteDataIt != _voteSnapshot.m_voteDataHistory.end(); _voteDataIt++)
    {
        std::map<u256, std::vector<PollData>>::const_iterator _it = _couplingSystemfee.m_sorted_creaters.find(_voteDataIt->first);
        std::map<u256, std::pair<u256, u256>>::const_iterator _amountIt = _couplingSystemfee.m_Feesnapshot.find(_voteDataIt->first + 1);
        if(_amountIt == _couplingSystemfee.m_Feesnapshot.end())
        {
            break;
        }
        std::vector<PollData> _PollDataV = _it->second;
        std::map<Address, u256> _superNodeAddrMap;
        u256 _totalPoll = 0;
        u256 _totalBrcFee = _amountIt->second.first;
        u256 _totalCookieFee = _amountIt->second.second;
        CFEE_LOG << " _totalBrcFee:" << _totalBrcFee << "   _totalCookieFee:" << _totalCookieFee;
        bool _isSuperNode = false;
        u256 _superNodePoll = 0;
        for(uint32_t i = 0; i < 7 && i < _PollDataV.size(); i++)
        {
            _totalPoll += _PollDataV[i].m_poll;
            if(_addr == _PollDataV[i].m_addr)
            {
                _isSuperNode = true;
                _superNodePoll = _PollDataV[i].m_poll;
            }
            _superNodeAddrMap[_PollDataV[i].m_addr] = _PollDataV[i].m_poll;
        }

        if(_isSuperNode == true)
        {
            u256 _Brc = _totalBrcFee / _totalPoll * _superNodePoll;
            u256 _Cookie = _totalCookieFee / _totalPoll * _superNodePoll;
            _BrcIncome += _Brc - (_Brc / 2 / _superNodePoll * _superNodePoll);
            _CookieIncome += _Cookie - (_Cookie / 2 /  _superNodePoll * _superNodePoll);
        }
        CFEE_LOG << "_BrcIncome : " << _BrcIncome << "   _CookieIncome:" << _CookieIncome;
        for(auto const& _voteAddressIt : _voteDataIt->second)
        {
            if(_superNodeAddrMap.count(_voteAddressIt.first))
            {
                std::map<Address, u256>::const_iterator _addrPollIt = _superNodeAddrMap.find(_voteAddressIt.first);
                u256 _currentNodeBrc = _totalBrcFee / _totalPoll * _addrPollIt->second;
                u256 _currentNodeCookie = _totalCookieFee / _totalPoll * _addrPollIt->second;
                _BrcIncome += (_currentNodeBrc / 2 / _addrPollIt->second * _voteAddressIt.second);
                _CookieIncome += (_currentNodeCookie / 2 / _addrPollIt->second * _voteAddressIt.second);
            }
        }
    }
    CFEE_LOG << "_BrcIncome : " << _BrcIncome << "   _CookieIncome:" << _CookieIncome;
    addBalance(_addr, _CookieIncome);
    addBRC(_addr, _BrcIncome);
    if(rounds)
    {
        a->set_numberofrounds(rounds);
    }

}

void State::anytime_receivingPdFeeIncome(const dev::Address &_addr, int64_t _blockNum){
    u256 total_income_cookies = 0;
    u256 total_income_brcs = 0;
    bool  is_update = false;
    auto a =  account(_addr);
    auto systemAccount = account(dev::PdSystemAddress);
    if(!a || !systemAccount){
        //throw ;
    }
//    std::map <u256, std::pair<u256, u256>> m_Feesnapshot;   // <rounds, <brc, cookies>>
//    std::map<u256, std::vector<PollData> > m_sorted_creaters;
//    u256 m_rounds = 0;
//    u256 m_numofrounds = 0;
//    std::map<u256, std::map<Address, std::pair<u256, u256>>> m_received_cookies;  // <rounds,<address, <total_summary, total_recived>>> recevied from other
//    std::map<u256, std::map<Address, std::pair<u256, u256>>> m_received_brcs;  // <rounds,<address, <total_summary, total_recived>>> recevied from other
    auto received_sanp = a->getFeeSnapshot();
    auto system_sanp = systemAccount->getFeeSnapshot();
    VoteSnapshot vote_sanp = a->vote_snashot();
    //CFEE_LOG << "vote_sanp:" << vote_sanp;

    std::pair<uint32_t, Votingstage> _pair = config::getVotingCycle(_blockNum);
    CouplingSystemfee fee_temp = a->getFeeSnapshot();
    //CFEE_LOG << " ......1....... fee_temp:" << fee_temp;

    /// calculate now vote_log and summary_income
    std::vector<PollData> now_miner;
    u256 now_total_poll = 0;
    uint32_t  num = config::minner_rank_num();
    for(auto const& val: vote_data(SysVarlitorAddress)){
        if (num){
            now_miner.emplace_back(val);
            now_total_poll += val.m_poll;
            --num;
        }
    }
    //summary_income
    u256 now_total_cookies = systemAccount->balance();
    u256 now_total_brcs = systemAccount->BRC();
    //auto old_sunmmary = received_sanp.m_total_summary();
    std::pair<u256, u256> old_summary = received_sanp.m_total_summary;
//    CFEE_LOG << "now_total_cookies:" << now_total_cookies;
//    CFEE_LOG << "now_total_brcs:" << now_total_brcs;
    bool is_now_rounds = false;
    int start_round = received_sanp.m_numofrounds != 0 ? (int)received_sanp.m_numofrounds : 1;
    /// loop any not received rounds . to now rounds
    for(int i=start_round; i<= _pair.first; i++) {
        is_now_rounds = (i == _pair.first);
        u256 round_cookies = 0;
        u256 round_brcs = 0;
        u256 _totalPoll = 0;
        std::pair<u256, u256> summary;
        std::vector<PollData> check_creater;
        do{
            if (!vote_sanp.m_voteDataHistory.count(i-1)){
                break;
            }
            if (!system_sanp.m_Feesnapshot.count(i) && !is_now_rounds){
                break;
            }
            if (!system_sanp.m_sorted_creaters.count(i-1) && !is_now_rounds){
                break;
            }
            if(is_now_rounds){
               _totalPoll = now_total_poll;
               summary = std::make_pair(now_total_brcs, now_total_cookies);
               check_creater = now_miner;
            } else {
                _totalPoll = system_sanp.get_total_poll(i, config::minner_rank_num());
                summary = system_sanp.m_Feesnapshot[i];
                check_creater = system_sanp.m_sorted_creaters[i];
            }
            if(summary.first == old_summary.first && summary.second == old_summary.second){
                old_summary = std::make_pair(0,0);
                continue;
            }

            //vote log for other
            std::map<Address, u256> vote_log = vote_sanp.m_voteDataHistory[i-1];
            /// loop all
            for(auto const& val: check_creater){
                // val = PollData
                u256 node_summary_cookies =  summary.second / _totalPoll * val.m_poll;
                u256 node_summary_brcs =  summary.first / _totalPoll * val.m_poll;
                u256 _income_cookies =0;
                u256 _income_brcs = 0;
                std::pair<u256, u256> _old_get; //<brc, cookies>

                if (received_sanp.m_received_cookies.count(i) && received_sanp.m_received_cookies[i].count(val.m_addr)){
                    _old_get = received_sanp.m_received_cookies[i][val.m_addr];
                }
                if(_old_get.second >= node_summary_cookies && _old_get.first >= node_summary_brcs){
                    break;
                }
                if (_addr== val.m_addr){
                    // super
                    _income_brcs += node_summary_brcs - (node_summary_brcs / 2 / val.m_poll *val.m_poll);
                    _income_cookies += node_summary_cookies - (node_summary_cookies / 2 / val.m_poll *val.m_poll);
                    //CFEE_LOG << " super:" << _income_brcs << "  "<< _income_cookies;
                }
                if (vote_log.count(val.m_addr)){
                    // vote node
                    _income_brcs += node_summary_brcs / 2 / val.m_poll * vote_log[val.m_addr];
                    _income_cookies += node_summary_cookies / 2 / val.m_poll * vote_log[val.m_addr];
                    //CFEE_LOG << " vote + super:" << _income_brcs << "  "<< _income_cookies;
                }
                //CFEE_LOG << " old-brcs-cookies"<< _old_get  << " _income_ brc--cookies :" <<_income_brcs <<"  "<< _income_cookies;
                fee_temp.up_received_cookies_brcs(i, val.m_addr, std::make_pair(_income_brcs, _income_cookies), summary);
                round_brcs += (_income_brcs - _old_get.first);
                round_cookies += (_income_cookies - _old_get.second);
                old_summary = std::make_pair(0,0);
                is_update = true;
            }
        }while (false);
        /// add received
        total_income_cookies += round_cookies;
        total_income_brcs += round_brcs;
        //CFEE_LOG << " total:" << total_income_brcs << "  "<< total_income_cookies;
    }
    if(is_update) {
        fee_temp.m_numofrounds = _pair.first;
        m_changeLog.emplace_back(dev::PdSystemAddress, a->getFeeSnapshot());
        a->setCouplingSystemFeeSnapshot(fee_temp);
        a->addBRC(total_income_brcs);
        a->addBalance(total_income_cookies);
    }
}

void State::createContract(Address const& _address)
{
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
        cwarn << BrcYellow "rollback: "
                  << " change.kind" << (size_t) change.kind << " change.value:" << change.value
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

void dev::brc::State::execute_vote(Address const & _addr, std::vector<std::shared_ptr<transationTool::operation> > const & _ops, EnvInfo const& info){

    u256 block_num = (u256)info.number();
	for(auto const& val : _ops){
		std::shared_ptr<transationTool::vote_operation> _p = std::dynamic_pointer_cast<transationTool::vote_operation>(val);
		if(!_p){
			cerror << "execute vote dynamic type field!";
			BOOST_THROW_EXCEPTION(InvalidDynamic());
		}
		VoteType _type = (VoteType)_p->m_vote_type;
		if(_type == VoteType::EBuyVote) {
		    try_new_vote_snapshot(_addr, block_num);
            transferBallotBuy(_addr, _p->m_vote_numbers);
        }
		else if(_type == VoteType::ESellVote) {
            try_new_vote_snapshot(_addr, block_num);
            transferBallotSell(_addr, _p->m_vote_numbers);
        }
		else if(_type == VoteType::ELoginCandidate) {
            try_new_vote_snapshot(_addr, block_num);
            addSysVoteDate(SysElectorAddress, _addr);
        }
		else if(_type == VoteType::ELogoutCandidate) {
            try_new_vote_snapshot(_addr, block_num);
            subSysVoteDate(SysElectorAddress, _addr);
        }
		else if(_type == VoteType::EDelegate) {
            try_new_vote_snapshot(_addr, block_num);
            try_new_vote_snapshot(_p->m_to, block_num);
            add_vote(_addr, {_p->m_to, (u256)_p->m_vote_numbers, info.timestamp()});
        }
		else if(_type == EUnDelegate) {
            try_new_vote_snapshot(_addr, block_num);
            try_new_vote_snapshot(_p->m_to, block_num);
            sub_vote(_addr, {_p->m_to, (u256)_p->m_vote_numbers, info.timestamp()});
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
        uint32_t  num =0;
        for (auto val : a->vote_data()) {
            if(num++ > config::max_message_num())   // limit message num
                break;
            Json::Value _v;
            _v["Address"] = toJS(val.m_addr);
            _v["vote_num"] = toJS(val.m_poll);
            _array.append(_v);
        }
        jv["vote"] = _array;
//
//        Json::Value record;
//        record["time"] = toJS(a->last_records(_addr));
//        jv["last_block_created"] = record;
    }
    return jv;
}


// Return to the block reward record in the form of paging
Json::Value dev::brc::State::blockRewardMessage(Address const& _addr, uint32_t const& _pageNum, uint32_t const& _listNum)
{
    try{
        Json::Value jv;
        if(auto a = account(_addr))
        {
            if(a->blockReward().size() > 0)
            {
                std::vector<std::pair<u256, u256>> _Vector = a->blockReward();
                uint32_t _retList = 0;
                if(_pageNum * _listNum > _Vector.size())
                {
                    _retList = _Vector.size();
                }else{
                    _retList = _pageNum * _listNum;
                }
                if(_pageNum == 0 || _listNum == 0)
                {
                    return jv;
                }
                if((_pageNum - 1) * _listNum > _Vector.size())
                {
                    return jv;
                }

                std::vector<std::pair<u256, u256>> res(_retList - ((_pageNum-1)* _listNum));
            
                if(_pageNum == 1)
                {
                    std::copy(_Vector.begin(), _Vector.begin()+_retList, res.begin());
                }else{
                    std::copy(_Vector.begin() + (_pageNum-1)*_listNum, _Vector.begin()+_retList, res.begin());
                }
                Json::Value _rewardArray; 
                for(auto it : res)
                {
                    Json::Value _vReward;
                    _vReward["blockNum"] = toJS(it.first);
                    _vReward["rewardNum"] = toJS(it.second);
                    _rewardArray.append(_vReward);
                }
                jv["BlockReward"] = _rewardArray;
            }
        }
        return jv;
    }catch(...)
    {
        throw;
    }
    
}

Json::Value dev::brc::State::votedMessage(Address const& _addr) const
{
	Json::Value jv;
	if(auto a = account(_addr))
	{
		Json::Value _arry;
		int _num = 0;
        uint32_t  num =0;
        for(auto val : a->vote_data())
		{
            if(num++ > config::max_message_num())   // limit message num
                break;
			Json::Value _v;
			_v["address"] = toJS(val.m_addr);
			_v["voted_num"] = toJS(val.m_poll);
			_arry.append(_v);
			_num += (int)val.m_poll;
		}
		jv["vote"] = _arry;
		jv["total_voted_num"] = toJS(_num);
	}
	return jv;
}

Json::Value dev::brc::State::electorMessage(Address _addr) const
{
	Json::Value jv;
	Json::Value _arry;
    const std::vector<PollData> _data = vote_data(SysElectorAddress);
	if(_addr == ZeroAddress)
	{
	    int num=0;
		for(auto val : _data)
		{
            if(num++ > config::max_message_num())   // limit message num
                break;
			Json::Value _v;
			_v["address"] = toJS(val.m_addr);
			_v["vote_num"] = toJS(val.m_poll);
			_arry.append(_v);
		}
		jv["electors"] = _arry;
	}
	else
	{
		jv["addrsss"] = toJS(_addr);
		auto ret = std::find(_data.begin(), _data.end(), _addr);
		auto a = account(_addr);
		if(ret != _data.end() && a)
		{
			jv["obtain_vote"] = toJS(a->poll());
		}
		else
			jv["ret"] = "not is the eletor";
	}
	return jv;
}

void dev::brc::State::systemPendingorder(int64_t _time)
{
    auto u256Safe = [](std::string const& s) -> u256 {
        bigint ret(s);
        if (ret >= bigint(1) << 256)
            BOOST_THROW_EXCEPTION(
                ValueTooLarge() << errinfo_comment("State value is equal or greater than 2**256"));
        return (u256)ret;
    };

	auto a = account(dev::VoteAddress);
	std::string _num = "2900000000000000";
    cwarn << "genesis pendingorder Num :" << _num;
	u256 systenCookie = u256Safe(_num);
	std::pair<u256, u256> _pair = {u256Safe(std::string("100000000")), systenCookie};
	order _order = { h256(1), dev::systemAddress, dev::brc::ex::order_buy_type::only_price, dev::brc::ex::order_token_type::FUEL, dev::brc::ex::order_type::sell, _pair, _time };
	std::vector<order> _v = { {_order} };

	try
	{
		m_exdb.insert_operation(_v, false, true);
	}
	catch (const boost::exception& e)
	{
		exit(1);
	}
	catch (...)
	{
		exit(1);
	}
//	m_exdb.commit(1, h256(), h256());
    m_exdb.rollback();
    m_exdb.commit_disk(1, true);
	cnote << m_exdb.check_version(false);
}

void dev::brc::State::add_vote(const dev::Address &_id, dev::brc::PollData const&p_data) {
    Address  _recivedAddr = p_data.m_addr;
    u256 _value = p_data.m_poll;
    Account *a = account(_id);
    Account *rec_a = account(_recivedAddr);
    Account *sys_a = account(SysElectorAddress);
    u256  _poll = 0;
    PollData poll_data ;
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
        m_changeLog.emplace_back(Change::Vote, _id, std::make_pair(_recivedAddr, 0-_value));
        m_changeLog.emplace_back(Change::SystemAddressPoll, _id, poll_data);
        m_changeLog.emplace_back(Change::Ballot, _id, 0 - _value);
        m_changeLog.emplace_back(Change::Poll, _recivedAddr, _value);
    }
}

void dev::brc::State::sub_vote(const dev::Address &_id, dev::brc::PollData const&p_data) {
    Address  _recivedAddr = p_data.m_addr;
    u256 _value = p_data.m_poll;
    Account *rec_a = account(_recivedAddr);
    Account *a = account(_id);
    Account *sys_a = account(SysElectorAddress);
    u256 _poll;
    PollData poll_data ;

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
        sys_a->set_system_poll({_recivedAddr, rec_a->poll(), poll_data.m_time > 0 ? poll_data.m_time: p_data.m_time});
    } else {
        cerror << "address error!";
        BOOST_THROW_EXCEPTION(InvalidAddressAddr() << errinfo_interface("State::subVote()"));
    }

    if (_value) {
        m_changeLog.emplace_back(Change::Vote,_id, std::make_pair(_recivedAddr, 0 - _value));
        m_changeLog.emplace_back(Change::SystemAddressPoll,_id, poll_data );
        m_changeLog.emplace_back(Change::Ballot, _id, _value);
        m_changeLog.emplace_back(Change::Poll, _recivedAddr, 0 - _value);
    }
}

const std::vector<PollData> dev::brc::State::vote_data(const dev::Address &_addr) const {
    if(auto a = account(_addr)){
        return  a->vote_data();
    }
    return std::vector<PollData>();
}

const PollData dev::brc::State::poll_data(const dev::Address &_addr, const dev::Address &_recv_addr) const{
    if(auto a = account(_addr)){
        return  a->poll_data(_recv_addr);
    }
    return  PollData();
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

void dev::brc::State::try_new_vote_snapshot(const dev::Address &_addr, dev::u256 _block_num) {
    std::pair<uint32_t, Votingstage> _pair = dev::brc::config::getVotingCycle((int64_t)_block_num);
    if (_pair.second == Votingstage::ERRORSTAGE)
        return ;
    auto  a = account(_addr);
    if(!a){
        createAccount(_addr, {0});
        a = account(_addr);
    }
    std::pair<bool, u256> ret_pair = a->get_no_record_snapshot((u256)_pair.first, _pair.second);
    if (!ret_pair.first){
        return ;
    }
    VoteSnapshot _vote_sna = a->vote_snashot();
    /// try new snapshot
    a->try_new_snapshot(ret_pair.second);

    /// clear genesis_vote_data and genesis_rounds poll
    if(_vote_sna.m_latest_round == 0){
        std::vector<PollData> poll_data= a->vote_data();
        a->clear_vote_data();
        subPoll(_addr, a->poll());
        m_changeLog.emplace_back(Change::MinnerSnapshot, _addr, poll_data);
    }

    m_changeLog.emplace_back(_addr, _vote_sna);
    m_changeLog.emplace_back(Change::CooikeIncomeNum, _addr, 0 - a->CookieIncome());

    setCookieIncomeNum(_addr, 0);
}

void dev::brc::State::tryRecordFeeSnapshot(int64_t _blockNum)
{
    std::pair<uint32_t, Votingstage> _pair = config::getVotingCycle(_blockNum);
    auto a = account(dev::PdSystemAddress);
    if(!a)
    {
        createAccount(dev::PdSystemAddress, {0});
        a = account(dev::PdSystemAddress);
    }
    u256 _rounds = a->getSnapshotRounds();
    if(_pair.first > _rounds ) {
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
        for (uint32_t i = 0; i < 7 && i < _pollDataV.size(); i++) {
            _snapshotTotalPoll += _pollDataV[i].m_poll;
        }

        if (_snapshotTotalPoll != 0)
        {
            remainder_brc = a->BRC()% _snapshotTotalPoll;
            remainder_ballance = a->balance()% _snapshotTotalPoll;
        }

        a->tryRecordSnapshot(_pair.first, a->BRC()- remainder_brc, a->balance() - remainder_ballance, vote_data(SysVarlitorAddress));

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
    subBRC(dev::systemAddress, _value * BALLOTPRICE );
}

int64_t dev::brc::State::last_block_record(Address const& _id) const{
    auto a = account(SysBolckCreateRecordAddress);
    if(!a){
        return 0;
    }
    return a->last_records(_id);

}

void dev::brc::State::set_last_block_record(const dev::Address &_id, const std::pair<dev::u256, int64_t> &value, uint32_t varlitor_time) {
    auto a = account(SysBolckCreateRecordAddress);
    if(!a){
        createAccount(SysBolckCreateRecordAddress, {0});
        a = account(SysBolckCreateRecordAddress);
    }
    int64_t _time = a->last_records(_id);
    a->set_create_record(std::make_pair(_id, value.second));

    if(value.first == 1){
        // if the first block will record all super_minner
        // bese for _id's offset to set other-time
        if(auto varlitor_a = account(SysVarlitorAddress)){
            // find offset
            int offset = 0;
            for(auto const& val: varlitor_a->vote_data()){
                if(val.m_addr == _id)
                    break;
                ++offset;
            }
            int index =0;
            for(auto const& val: varlitor_a->vote_data()){
                if (val.m_addr != _id){
                    int64_t  _time= value.second +  (index-offset) * (int)varlitor_time;
                    a->set_create_record(std::make_pair(val.m_addr, _time));
                }
                ++index;
            }
        }
    }
    m_changeLog.emplace_back(Change::LastCreateRecord, _id, std::make_pair(_id, _time));
}

BlockRecord dev::brc::State::block_record() const {
    auto  a= account(SysBolckCreateRecordAddress);
    if (!a)
        return BlockRecord();
    return  a->block_record();
}

void dev::brc::State::try_newrounds_count_vote(const dev::brc::BlockHeader &curr_header, const dev::brc::BlockHeader &previous_header) {
    //testlog << "curr:"<< curr_header.number() << "  pre:"<< previous_header.number();
    std::pair<uint32_t, Votingstage> previous_pair = dev::brc::config::getVotingCycle(previous_header.number());
    std::pair<uint32_t, Votingstage> curr_pair = dev::brc::config::getVotingCycle(curr_header.number());
//    if (previous_header.number() >= curr_header.number())
//        return;
//    if (curr_pair.second != Votingstage::RECEIVINGINCOME || curr_pair.second == previous_pair.second)
//        return;
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
    if (!varlitor_a){
        createAccount(SysVarlitorAddress, {0});
        varlitor_a = account(SysVarlitorAddress);
    }
    auto standby_a = account(SysCanlitorAddress);
    if (!standby_a) {
        createAccount(SysCanlitorAddress, {0});
        standby_a = account(SysCanlitorAddress);
    }

    std::vector<PollData> p_data = a->vote_data();
    //std::sort(p_data.begin(), p_data.end(), std::greater<struct PollData>());
    PollData::sort_greater(p_data);

    u256 var_num = config::varlitorNum();
    u256 standby_num = config::standbyNum();
    std::vector<PollData> varlitors;
    std::vector<PollData> standbys;

    for(auto const& val: p_data){
        if (var_num){
            varlitors.emplace_back(val);
            var_num --;
        } else if (standby_num){
            standbys.emplace_back(val);
            standby_num--;
        }
    }
    if (varlitors.empty())
        return;

    ///add sanpshot about miner and standby
    auto minersanp_a = account(SysMinerSnapshotAddress);
    if (!minersanp_a){
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

std::map<u256, std::vector<PollData>> dev::brc::State::get_miner_snapshot() const{
    auto minersanp_a = account(SysMinerSnapshotAddress);
    if (!minersanp_a){
        return std::map<u256, std::vector<PollData>>();
    }
    return  minersanp_a->getFeeSnapshot().m_sorted_creaters;
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
AddressHash dev::brc::commit(AccountMap const &_cache, SecureTrieDB<Address, DB> &_state, uint64_t _time /*= FORKSIGNSTIME*/) {
    AddressHash ret;
    for (auto const &i : _cache)
        if (i.second.isDirty()) {
            if (!i.second.isAlive())
                _state.remove(i.first);
            else {
				RLPStream s;
				s.appendList(17);
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
                    for(auto const& val: i.second.vote_data()){
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
                    for (auto it : i.second.blockReward())
                    {
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
                _state.insert(i.first, &s.out());
            }
            ret.insert(i.first);
        }
    return ret;
}

template AddressHash dev::brc::commit<OverlayDB>(
        AccountMap const &_cache, SecureTrieDB<Address, OverlayDB> &_state, uint64_t _time = FORKSIGNSTIME);

template AddressHash dev::brc::commit<StateCacheDB>(
        AccountMap const &_cache, SecureTrieDB<Address, StateCacheDB> &_state, uint64_t _time = FORKSIGNSTIME);
