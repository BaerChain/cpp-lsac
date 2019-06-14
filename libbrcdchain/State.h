#pragma once

#include "Account.h"
#include "GasPricer.h"
#include "SecureTrieDB.h"
#include "Transaction.h"
#include "TransactionReceipt.h"
#include <libbrccore/BlockHeader.h>
#include <libbrccore/Exceptions.h>
#include <libbrccore/SealEngine.h>
#include <libbrcdchain/CodeSizeCache.h>
#include <libbrccore/SealEngine.h>
#include <libbvm/ExtVMFace.h>
#include <libdevcore/Common.h>
#include <libdevcore/OverlayDB.h>
#include <libdevcore/RLP.h>
#include <array>
#include <brc/exchangeOrder.hpp>
#include <brc/types.hpp>
#include <unordered_map>

namespace dev
{
namespace test
{
class ImportTest;
class StateLoader;
}  // namespace test

namespace brc
{
// Import-specific errinfos
using errinfo_uncleIndex = boost::error_info<struct tag_uncleIndex, unsigned>;
using errinfo_currentNumber = boost::error_info<struct tag_currentNumber, u256>;
using errinfo_uncleNumber = boost::error_info<struct tag_uncleNumber, u256>;
using errinfo_unclesExcluded = boost::error_info<struct tag_unclesExcluded, h256Hash>;
using errinfo_block = boost::error_info<struct tag_block, bytes>;
using errinfo_now = boost::error_info<struct tag_now, unsigned>;

using errinfo_transactionIndex = boost::error_info<struct tag_transactionIndex, unsigned>;

using errinfo_vmtrace = boost::error_info<struct tag_vmtrace, std::string>;
using errinfo_receipts = boost::error_info<struct tag_receipts, std::vector<bytes>>;
using errinfo_transaction = boost::error_info<struct tag_transaction, bytes>;
using errinfo_phase = boost::error_info<struct tag_phase, unsigned>;
using errinfo_required_LogBloom = boost::error_info<struct tag_required_LogBloom, LogBloom>;
using errinfo_got_LogBloom = boost::error_info<struct tag_get_LogBloom, LogBloom>;
using LogBloomRequirementError = boost::tuple<errinfo_required_LogBloom, errinfo_got_LogBloom>;

class BlockChain;
class State;
class TransactionQueue;
struct VerifiedBlockRef;

enum class BaseState
{
    PreExisting,
    Empty
};

enum class Permanence
{
    Reverted,
    Committed,
    Uncommitted  ///< Uncommitted state for change log readings in tests.
};

DEV_SIMPLE_EXCEPTION(InvalidAccountStartNonceInState);
DEV_SIMPLE_EXCEPTION(IncorrectAccountStartNonceInState);
DEV_SIMPLE_EXCEPTION(InvalidAddressAddr);
DEV_SIMPLE_EXCEPTION(NotEnoughPoll);
DEV_SIMPLE_EXCEPTION(InvalidAddressAddVote);
DEV_SIMPLE_EXCEPTION(NotEnoughVoteLog);
DEV_SIMPLE_EXCEPTION(InvalidSysAddress);
DEV_SIMPLE_EXCEPTION(InvalidDynamic);

class SealEngineFace;
class Executive;

/// An atomic state changelog entry.
struct Change
{
    enum Kind : int
    {
        /// Account balance changed. Change::value contains the amount the
        /// balance was increased by.
        Balance,
        /// Account storage was modified. Change::key contains the storage key,
        /// Change::value the storage value.
        Storage,

        /// Account storage root was modified.  Change::value contains the old
        /// account storage root.
        StorageRoot,

        /// Account nonce was changed.
        Nonce,

        /// Account was created (it was not existing before).
        Create,

        /// New code was added to an account (by "create" message execution).
        Code,

        /// Account was touched for the first time.
        Touch,

        Ballot,
        Poll,
        Vote,
        SysVoteData,
        BRC,
        FBRC,               // = 12
        FBalance,
        BlockReward
    };

    Kind kind;        ///< The kind of the change.
    Address address;  ///< Changed account address.
    u256 value;       ///< Change value, e.g. balance, storage and nonce.
    u256 key;         ///< Storage key. Last because used only in one case.
    bytes oldCode;    ///< Code overwritten by CREATE, empty except in case of address collision.
    std::pair<Address, u256> vote;         // 投票事件
    std::pair<Address, bool> sysVotedate;  // 成为/撤销竞选人事件
    std::pair<u256, u256> blockReward;

    /// Helper constructor to make change log update more readable.
    Change(Kind _kind, Address const& _addr, u256 const& _value = 0)
      : kind(_kind), address(_addr), value(_value)
    {
        assert(_kind != Code);  // For this the special constructor needs to be used.
    }

    /// Helper constructor especially for storage change log.
    Change(Address const& _addr, u256 const& _key, u256 const& _value)
      : kind(Storage), address(_addr), value(_value), key(_key)
    {}

    /// Helper constructor for nonce change log.
    Change(Address const& _addr, u256 const& _value) : kind(Nonce), address(_addr), value(_value) {}

    /// Helper constructor especially for new code change log.
    Change(Address const& _addr, bytes const& _oldCode)
      : kind(Code), address(_addr), oldCode(_oldCode)
    {}

    //  Helper constructor for vote change log
    Change(Address const& _addr, std::pair<Address, u256> _vote) : kind(Vote), address(_addr)
    {
        vote = std::make_pair(_vote.first, _vote.second);
    }
    Change(Address const& _addr, std::pair<Address, bool> _sysVote) : kind(Vote), address(_addr)
    {
        sysVotedate = std::make_pair(_sysVote.first, _sysVote.second);
    }
    Change(Address const& _addr, std::pair<u256, u256> _pair) : kind(BlockReward), address(_addr)
    {
        blockReward = std::make_pair(_pair.first, _pair.second);
    }
};

using ChangeLog = std::vector<Change>;

/**
 * Model of an BrcdChain state, essentially a facade for the trie.
 *
 * Allows you to query the state of accounts as well as creating and modifying
 * accounts. It has built-in caching for various aspects of the state.
 *
 * # State Changelog
 *
 * Any atomic change to any account is registered and appended in the changelog.
 * In case some changes must be reverted, the changes are popped from the
 * changelog and undone. For possible atomic changes list @see Change::Kind.
 * The changelog is managed by savepoint(), rollback() and commit() methods.
 */
class State
{
    friend class ExtVM;
    friend class dev::test::ImportTest;
    friend class dev::test::StateLoader;
    friend class BlockChain;
    friend class DposVote;
    friend class BRCTranscation;

public:
    enum class CommitBehaviour
    {
        KeepEmptyAccounts,
        RemoveEmptyAccounts
    };

    using AddressMap = std::map<h256, Address>;

    /// Default constructor; creates with a blank database prepopulated with the genesis block.
    explicit State(u256 const& _accountStartNonce)
      : State(_accountStartNonce, OverlayDB(), ex::exchange_plugin(), BaseState::Empty)
    {}

    /// Basic state object from database.
    /// Use the default when you already have a database and you just want to make a State object
    /// which uses it. If you have no preexisting database then set BaseState to something other
    /// than BaseState::PreExisting in order to prepopulate the Trie.
    explicit State(u256 const& _accountStartNonce, OverlayDB const& _db, ex::exchange_plugin const& _exdb,
        BaseState _bs = BaseState::PreExisting);

    enum NullType
    {
        Null
    };
    State(NullType) : State(Invalid256, OverlayDB(), ex::exchange_plugin(), BaseState::Empty) {}

    /// Copy state object.
    State(State const& _s);

    /// Copy state object.
    State& operator=(State const& _s);

    /// Open a DB - useful for passing into the constructor & keeping for other states that are
    /// necessary.
    static OverlayDB openDB(boost::filesystem::path const& _path, h256 const& _genesisHash, WithExisting _we = WithExisting::Trust);
    OverlayDB const& db() const { return m_db; }
    OverlayDB& db() { return m_db; }

    static ex::exchange_plugin openExdb(boost::filesystem::path const& _path, WithExisting _we = WithExisting::Trust);
    ex::exchange_plugin const& exdb() const { return m_exdb; }
    ex::exchange_plugin& exdb() { return m_exdb; }

    /// Populate the state from the given AccountMap. Just uses dev::brc::commit().
    void populateFrom(AccountMap const& _map);

    /// @returns the set containing all addresses currently in use in BrcdChain.
    /// @warning This is slowslowslow. Don't use it unless you want to lock the object for seconds
    /// or minutes at a time.
    /// @throws InterfaceNotSupported if compiled without BRC_FATDB.
    std::unordered_map<Address, u256> addresses() const;

    /// @returns the map with maximum _maxResults elements containing hash->addresses and the next
    /// address hash. This method faster then addresses() const;
    std::pair<AddressMap, h256> addresses(h256 const& _begin, size_t _maxResults) const;

    /// Execute a given transaction.
    /// This will change the state accordingly.
    std::pair<ExecutionResult, TransactionReceipt> execute(EnvInfo const& _envInfo,
        SealEngineFace const& _sealEngine, Transaction const& _t,
        Permanence _p = Permanence::Committed, OnOpFunc const& _onOp = OnOpFunc());

    /// Execute @a _txCount transactions of a given block.
    /// This will change the state accordingly.
    void executeBlockTransactions(Block const& _block, unsigned _txCount,
        LastBlockHashesFace const& _lastHashes, SealEngineFace const& _sealEngine);

    /// Check if the address is in use.
    bool addressInUse(Address const& _address) const;

    /// Check if the account exists in the state and is non empty (nonce > 0 || balance > 0 || code
    /// nonempty). These two notions are equivalent after EIP158.
    bool accountNonemptyAndExisting(Address const& _address) const;

    /// Check if the address contains executable code.
    bool addressHasCode(Address const& _address) const;

    /// Get an account's balance.
    /// @returns 0 if the address has never been used.
    u256 balance(Address const& _id) const;

    /// Add some amount to balance.
    /// Will initialise the address if it has never been used.
    void addBalance(Address const& _id, u256 const& _amount);

    /// Subtract the @p _value amount from the balance of @p _addr account.
    /// @throws NotEnoughCash if the balance of the account is less than the
    /// amount to be subtrackted (also in case the account does not exist).
    void subBalance(Address const& _addr, u256 const& _value);

    /// Set the balance of @p _addr to @p _value.
    /// Will instantiate the address if it has never been used.
    void setBalance(Address const& _addr, u256 const& _value);

    // BRC相关接口
    u256 BRC(Address const& _id) const;
    void addBRC(Address const& _addr, u256 const& _value);
    void subBRC(Address const& _addr, u256 const& _value);
    void setBRC(Address const& _addr, u256 const& _value);

    void transferBRC(Address const& _FromAddr, Address const& _ToAddr, u256 _value)
    {
        subBRC(_FromAddr, _value);
        addBRC(_ToAddr, _value);
    }
	void execute_transfer_BRCs(Address const& _addr, std::vector<std::shared_ptr<transationTool::operation> > const& _ops)
	{
	    for(auto const& val : _ops){
			std::shared_ptr<transationTool::transcation_operation> _p = std::dynamic_pointer_cast<transationTool::transcation_operation>(val);
			if(!_p){
				cerror << "execute_t transafer_brc dynamic type field!";
				BOOST_THROW_EXCEPTION(InvalidDynamic());
			}
			transferBRC(_addr, _p->m_to, _p->m_Transcation_numbers);
		}
	}

    // FBRC相关接口
    u256 FBRC(Address const& _id) const;
    void addFBRC(Address const& _addr, u256 const& _value);
    void subFBRC(Address const& _addr, u256 const& _value);

    // FBalance相关接口
    u256 FBalance(Address const& _id) const;
    void addFBalance(Address const& _addr, u256 const& _value);
    void subFBalance(Address const& _addr, u256 const& _value);


    //交易挂单接口
    void pendingOrder(Address const& _addr, u256 _pendingOrderNum, u256 _pendingOrderPrice,
        h256 _pendingOrderHash, ex::order_type _pendingOrderType, ex::order_token_type _pendingOrderTokenType,
        ex::order_buy_type _pendingOrderBuyType, int64_t _nowTime);

	void pendingOrders(Address const& _addr, int64_t _nowTime, h256 _pendingOrderHash, std::vector<std::shared_ptr<transationTool::operation>> const& _ops);

    void cancelPendingOrder(h256 _pendingOrderHash);
	void cancelPendingOrders(std::vector<std::shared_ptr<transationTool::operation>> const& _ops);


	void freezeAmount(Address const& _addr, u256 _pendingOrderNum, u256 _pendingOrderPrice,
		ex::order_type _pendingOrderType, ex::order_token_type _pendingOrderTokenType, ex::order_buy_type _pendingOrderBuyType);

	void pendingOrderTransfer(Address const& _from, Address const& _to, u256 _toPendingOrderNum,
        u256 _toPendingOrderPrice, ex::order_type _pendingOrderType, ex::order_token_type _pendingOrderTokenType,
        ex::order_buy_type _pendingOrderBuyTypes);

	Json::Value pendingOrderPoolMsg(uint8_t _order_type, uint8_t _order_token_type, u256 getSize);

	Json::Value pendingOrderPoolForAddrMsg(Address _a, uint32_t _getSize);

	Json::Value successPendingOrderMsg(uint32_t _getSize);

	std::tuple<std::string, std::string, std::string> enumToString(ex::order_type _type, ex::order_token_type _token_type, ex::order_buy_type _buy_type);



    //投票数相关接口 自己拥有可以操作的票数
    u256 ballot(Address const& _id) const;
    void addBallot(Address const& _addr, u256 const& _value);
    void subBallot(Address const& _id, u256 const& _value);

    //得票数据接口
    u256 poll(Address const& _addr) const;
    void addPoll(Address const& _addr, u256 const& _value);
    void subPoll(Address const& _adddr, u256 const& _value);

	void execute_vote(Address const& _addr, std::vector<std::shared_ptr<transationTool::operation> > const& _ops);

    // 详细信息 test
    Json::Value accoutMessage(Address const& _addr);
    Json::Value blockRewardMessage(Address const& _addr, uint32_t const& _pageNum, uint32_t const& _listNum);
	//get vote/eletor message
	Json::Value votedMessage(Address const& _addr) const;
	Json::Value electorMessage(Address _addr) const;


	void assetInjection(Address const& _addr);

	void systemPendingorder(int64_t _time);
	void addBlockReward(Address const & _addr, u256 _blockNum, u256 _rewardNum);

private:
    //投票数据
    //获取投出得票数
    u256 voteAll(Address const& _id) const;
    //获取给指定Address的投票数
    u256 voteAdress(Address const& _id, Address const& _recivedAddr) const;
    //投票
    void addVote(Address const& _id, Address const& _recivedAddr, u256 _value);
    //撤销投票
    void subVote(Address const& _id, Address const& _recivedAddr, u256 _value);
    //获取指定地址的voteDate
    std::map<Address, u256> voteDate(Address const& _id) const;
    //竞选人，验证人管理
    void addSysVoteDate(Address const& _sysAddress, Address const& _id);
    void subSysVoteDate(Address const& _sysAddress, Address const& _id);

public:
    void transferBallotBuy(Address const& _from, u256 const& _value);
    void transferBallotSell(Address const& _from, u256 const& _value);
    // void transferBallot(Address const& _from, Address const& _to, u256 const& _value) {
    // subBallot(_from, _value); addBallot(_to, _value); }
    /**
     * @brief Transfers "the balance @a _value between two accounts.
     * @param _from Account from which @a _value will be deducted.
     * @param _to Account to which @a _value will be added.
     * @param _value Amount to be transferred.
     */
    void transferBalance(Address const& _from, Address const& _to, u256 const& _value)
    {

		//TO DO : Do not allow ordinary users to transfer money
        subBalance(_from, _value);
        addBalance(_to, _value);
    }

    /// Get the root of the storage of an account.
    h256 storageRoot(Address const& _contract) const;

    /// Get the value of a storage position of an account.
    /// @returns 0 if no account exists at that address.
    u256 storage(Address const& _contract, u256 const& _memory) const;

    /// Set the value of a storage position of an account.
    void setStorage(Address const& _contract, u256 const& _location, u256 const& _value);

    /// Get the original value of a storage position of an account (before modifications saved in
    /// account cache).
    /// @returns 0 if no account exists at that address.
    u256 originalStorageValue(Address const& _contract, u256 const& _key) const;

    /// Clear the storage root hash of an account to the hash of the empty trie.
    void clearStorage(Address const& _contract);

    /// Create a contract at the given address (with unset code and unchanged balance).
    void createContract(Address const& _address);

    /// Sets the code of the account. Must only be called during / after contract creation.
    void setCode(Address const& _address, bytes&& _code);

    /// Delete an account (used for processing suicides).
    void kill(Address _a);

    /// Get the storage of an account.
    /// @note This is expensive. Don't use it unless you need to.
    /// @returns map of hashed keys to key-value pairs or empty map if no account exists at that
    /// address.
    std::map<h256, std::pair<u256, u256>> storage(Address const& _contract) const;

    /// Get the code of an account.
    /// @returns bytes() if no account exists at that address.
    /// @warning The reference to the code is only valid until the access to
    ///          other account. Do not keep it.
    bytes const& code(Address const& _addr) const;

    /// Get the code hash of an account.
    /// @returns EmptySHA3 if no account exists at that address or if there is no code associated
    /// with the address.
    h256 codeHash(Address const& _contract) const;

    /// Get the byte-size of the code of an account.
    /// @returns code(_contract).size(), but utilizes CodeSizeHash.
    size_t codeSize(Address const& _contract) const;

    /// Increament the account nonce.
    void incNonce(Address const& _id);

    /// Set the account nonce.
    void setNonce(Address const& _addr, u256 const& _newNonce);

    /// Get the account nonce -- the number of transactions it has sent.
    /// @returns 0 if the address has never been used.
    u256 getNonce(Address const& _addr) const;

    /// The hash of the root of our state tree.
    h256 rootHash() const { return m_state.root(); }

    /// Commit all changes waiting in the address cache to the DB.
    /// @param _commitBehaviour whether or not to remove empty accounts during commit.
    void commit(CommitBehaviour _commitBehaviour);

    /// Resets any uncommitted changes to the cache.
    void setRoot(h256 const& _root);

    /// Get the account start nonce. May be required.
    u256 const& accountStartNonce() const { return m_accountStartNonce; }
    u256 const& requireAccountStartNonce() const;
    void noteAccountStartNonce(u256 const& _actual);

    /// Create a savepoint in the state changelog.
    /// @return The savepoint index that can be used in rollback() function.
    size_t savepoint() const;

    /// Revert all recent changes up to the given @p _savepoint savepoint.
    void rollback(size_t _savepoint);

    ChangeLog const& changeLog() const { return m_changeLog; }

private:
    /// Turns all "touched" empty accounts into non-alive accounts.
    void removeEmptyAccounts();

    /// @returns the account at the given address or a null pointer if it does not exist.
    /// The pointer is valid until the next access to the state or account.
    Account const* account(Address const& _addr) const;

    /// @returns the account at the given address or a null pointer if it does not exist.
    /// The pointer is valid until the next access to the state or account.
    Account* account(Address const& _addr);

    /// Purges non-modified entries in m_cache if it grows too large.
    void clearCacheIfTooLarge() const;

    void createAccount(Address const& _address, Account const&& _account);

    /// @returns true when normally halted; false when exceptionally halted; throws when internal VM
    /// exception occurred.
    bool executeTransaction(Executive& _e, Transaction const& _t, OnOpFunc const& _onOp);

    /// Our overlay for the state tree.
    OverlayDB m_db;

    // Exdb
    ex::exchange_plugin m_exdb;
    /// Our state tree, as an OverlayDB DB.
    SecureTrieDB<Address, OverlayDB> m_state;
    /// Our address cache. This stores the states of each address that has (or at least might have)
    /// been changed.
    mutable std::unordered_map<Address, Account> m_cache;
    /// Tracks entries in m_cache that can potentially be purged if it grows too large.
    mutable std::vector<Address> m_unchangedCacheEntries;
    /// Tracks addresses that are known to not exist.
    mutable std::set<Address> m_nonExistingAccountsCache;
    /// Tracks all addresses touched so far.
    AddressHash m_touched;

    u256 m_accountStartNonce;

    friend std::ostream& operator<<(std::ostream& _out, State const& _s);
    ChangeLog m_changeLog;

    mutable Logger m_loggerError{createLogger(VerbosityError, "State")};
};

std::ostream& operator<<(std::ostream& _out, State const& _s);

State& createIntermediateState(
    State& o_s, Block const& _block, unsigned _txIndex, BlockChain const& _bc);

template <class DB>
AddressHash commit(AccountMap const& _cache, SecureTrieDB<Address, DB>& _state);

}  // namespace brc
}  // namespace dev
