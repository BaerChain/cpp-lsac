#include "Brc.h"
#include "AccountHolder.h"
#include <jsonrpccpp/common/exception.h>
#include <libbrccore/CommonJS.h>
#include <libbrcdchain/Client.h>
#include <libbrchashseal/BrchashClient.h>
#include <libdevcore/CommonData.h>
#include <libweb3jsonrpc/JsonHelper.h>
#include <libwebthree/WebThree.h>
#include <csignal>

using namespace std;
using namespace jsonrpc;
using namespace dev;
using namespace brc;
using namespace shh;
using namespace dev::rpc;

Brc::Brc(brc::Interface& _brc, brc::AccountHolder& _brcAccounts)
  : m_brc(_brc), m_brcAccounts(_brcAccounts)
{}

string Brc::brc_protocolVersion()
{
    return toJS(brc::c_protocolVersion);
}

string Brc::brc_coinbase()
{
    return toJS(client()->author());
}

string Brc::brc_hashrate()
{
    try
    {
        return toJS(asBrchashClient(client())->hashrate());
    }
    catch (InvalidSealEngine&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

bool Brc::brc_mining()
{
    try
    {
        return asBrchashClient(client())->isMining();
    }
    catch (InvalidSealEngine&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_gasPrice()
{
    return toJS(client()->gasBidPrice());
}

Json::Value Brc::brc_accounts()
{
    return toJson(m_brcAccounts.allAccounts());
}

string Brc::brc_blockNumber()
{
    return toJS(client()->number());
}


Json::Value Brc::brc_getSuccessPendingOrder(string const& _getSize, string const& _blockNum)
{
	try
	{
		return client()->successPendingOrderMessage(jsToInt(_getSize), jsToBlockNumber(_blockNum));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Brc::brc_getPendingOrderPoolForAddr(
    string const& _address, string const& _getSize, string const& _blockNum)
{
	try
	{
        return client()->pendingOrderPoolForAddrMessage(
            jsToAddress(_address), jsToInt(_getSize), jsToBlockNumber(_blockNum));
	}
	catch (...)
	{
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Brc::brc_getPendingOrderPool(string const& _order_type, string const& _order_token_type,
    string const& _getSize, string const& _blockNumber)
{
    try
    {
        return client()->pendingOrderPoolMessage(jsToOrderEnum(_order_type),
            jsToOrderEnum(_order_token_type), jsToU256(_getSize), jsToBlockNumber(_blockNumber));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getBalance(string const& _address, string const& _blockNumber)
{
    try
    {
        // return toJS(client()->balanceAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
        return client()->accountMessage(jsToAddress(_address), jsToBlockNumber(_blockNumber));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_getBallot(string const& _address, string const& _blockNumber)
{
    try
    {
        return toJS(client()->ballotAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_getStorageAt(
    string const& _address, string const& _position, string const& _blockNumber)
{
    try
    {
        return toJS(toCompactBigEndian(client()->stateAt(jsToAddress(_address), jsToU256(_position),
                                           jsToBlockNumber(_blockNumber)),
            32));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_getStorageRoot(string const& _address, string const& _blockNumber)
{
    try
    {
        return toString(
            client()->stateRootAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_pendingTransactions()
{
    // Return list of transaction that being sent by local accounts
    Transactions ours;
    for (Transaction const& pending : client()->pending())
    {
        // for (Address const& account:m_brcAccounts.allAccounts())
        //{
        // if (pending.sender() == account)
        //{
        ours.push_back(pending);
        // break;
        //}
        //}
    }

    return toJson(ours);
}

string Brc::brc_getTransactionCount(string const& _address, string const& _blockNumber)
{
    try
    {
        return toJS(client()->countAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getBlockTransactionCountByHash(string const& _blockHash)
{
    try
    {
        h256 blockHash = jsToFixed<32>(_blockHash);
        if (!client()->isKnown(blockHash))
            return Json::Value(Json::nullValue);

        return toJS(client()->transactionCount(blockHash));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getBlockTransactionCountByNumber(string const& _blockNumber)
{
    try
    {
        BlockNumber blockNumber = jsToBlockNumber(_blockNumber);
        if (!client()->isKnown(blockNumber))
            return Json::Value(Json::nullValue);

        return toJS(client()->transactionCount(jsToBlockNumber(_blockNumber)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getUncleCountByBlockHash(string const& _blockHash)
{
    try
    {
        h256 blockHash = jsToFixed<32>(_blockHash);
        if (!client()->isKnown(blockHash))
            return Json::Value(Json::nullValue);

        return toJS(client()->uncleCount(blockHash));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getUncleCountByBlockNumber(string const& _blockNumber)
{
    try
    {
        BlockNumber blockNumber = jsToBlockNumber(_blockNumber);
        if (!client()->isKnown(blockNumber))
            return Json::Value(Json::nullValue);

        return toJS(client()->uncleCount(blockNumber));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_getCode(string const& _address, string const& _blockNumber)
{
    try
    {
        return toJS(client()->codeAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

void Brc::setTransactionDefaults(TransactionSkeleton& _t)
{
    if (!_t.from)
        _t.from = m_brcAccounts.defaultTransactAccount();
}

string Brc::brc_sendTransaction(Json::Value const& _json)
{
    try
    {
        TransactionSkeleton t = toTransactionSkeleton(_json);
        setTransactionDefaults(t);
        pair<bool, Secret> ar = m_brcAccounts.authenticate(t);
        if (!ar.first)
        {
            h256 txHash = client()->submitTransaction(t, ar.second);
            return toJS(txHash);
        }
        else
        {
            m_brcAccounts.queueTransaction(t);
            h256 emptyHash;
            return toJS(emptyHash);  // TODO: give back something more useful than an empty hash.
        }
    }
    catch (Exception const&)
    {
        throw JsonRpcException(exceptionToErrorMessage());
    }
}

Json::Value Brc::brc_signTransaction(Json::Value const& _json)
{
    try
    {
        TransactionSkeleton ts = toTransactionSkeleton(_json);
        setTransactionDefaults(ts);
        ts = client()->populateTransactionWithDefaults(ts);
        pair<bool, Secret> ar = m_brcAccounts.authenticate(ts);
        Transaction t(ts, ar.second);
        RLPStream s;
        t.streamRLP(s);
        return toJson(t, s.out());
    }
    catch (Exception const&)
    {
        throw JsonRpcException(exceptionToErrorMessage());
    }
}

Json::Value Brc::brc_inspectTransaction(std::string const& _rlp)
{
    try
    {
        return toJson(Transaction(jsToBytes(_rlp, OnFailed::Throw), CheckTransaction::Everything));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_sendRawTransaction(std::string const& _rlp)
{
    try
    {
        // Don't need to check the transaction signature (CheckTransaction::None) since it will
        // be checked as a part of transaction import
        Transaction t(jsToBytes(_rlp, OnFailed::Throw), CheckTransaction::None);
        return toJS(client()->importTransaction(t));
    }
    catch (Exception const& ex)
    {
		testlog << " error:" << ex.what();
        throw JsonRpcException(exceptionToErrorMessage());
    }
}

std::string Brc::brc_importBlock(const std::string &_rlp) {
    try {
        auto by = jsToBytes(_rlp, OnFailed::Throw);
        return toJS(client()->importBlock(&by));
    }catch (const Exception &e){
        testlog << " error:" << e.what();
        throw JsonRpcException(exceptionToErrorMessage());
    }
}

string Brc::brc_call(Json::Value const& _json, string const& _blockNumber)
{
    try
    {
        TransactionSkeleton t = toTransactionSkeleton(_json);
        setTransactionDefaults(t);
        ExecutionResult er = client()->call(t.from, t.value, t.to, t.data, t.gas, t.gasPrice,
            jsToBlockNumber(_blockNumber), FudgeFactor::Lenient);
		testlog << BrcYellow << er.output << BrcReset;
        return toJS(er.output);
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_estimateGas(Json::Value const& _json)
{
    try
    {
        TransactionSkeleton t = toTransactionSkeleton(_json);
        setTransactionDefaults(t);
        int64_t gas = static_cast<int64_t>(t.gas);
        return toJS(client()
                        ->estimateGas(t.from, t.value, t.to, t.data, gas, t.gasPrice, PendingBlock)
                        .first);
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

bool Brc::brc_flush()
{
    client()->flushTransactions();
    return true;
}

Json::Value Brc::brc_getBlockByHash(string const& _blockHash, bool _includeTransactions)
{
    try
    {
        h256 h = jsToFixed<32>(_blockHash);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactions(h), client()->sealEngine());
        else
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getBlockByNumber(string const& _blockNumber, bool _includeTransactions)
{
    try
    {
        BlockNumber h = jsToBlockNumber(_blockNumber);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactions(h), client()->sealEngine());
        else
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getTransactionByHash(string const& _transactionHash)
{
    try
    {
        h256 h = jsToFixed<32>(_transactionHash);
        if (!client()->isKnownTransaction(h))
            return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransaction(h));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getTransactionByBlockHashAndIndex(
    string const& _blockHash, string const& _transactionIndex)
{
    try
    {
        h256 bh = jsToFixed<32>(_blockHash);
        unsigned ti = jsToInt(_transactionIndex);
        if (!client()->isKnownTransaction(bh, ti))
            return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransaction(bh, ti));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getTransactionByBlockNumberAndIndex(
    string const& _blockNumber, string const& _transactionIndex)
{
    try
    {
        BlockNumber bn = jsToBlockNumber(_blockNumber);
        h256 bh = client()->hashFromNumber(bn);
        unsigned ti = jsToInt(_transactionIndex);
        if (!client()->isKnownTransaction(bh, ti))
            return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransaction(bh, ti));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getTransactionReceipt(string const& _transactionHash)
{
    try
    {
        h256 h = jsToFixed<32>(_transactionHash);
        if (!client()->isKnownTransaction(h))
            return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransactionReceipt(h));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getUncleByBlockHashAndIndex(
    string const& _blockHash, string const& _uncleIndex)
{
    try
    {
        return toJson(client()->uncle(jsToFixed<32>(_blockHash), jsToInt(_uncleIndex)),
            client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getUncleByBlockNumberAndIndex(
    string const& _blockNumber, string const& _uncleIndex)
{
    try
    {
        return toJson(client()->uncle(jsToBlockNumber(_blockNumber), jsToInt(_uncleIndex)),
            client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_newFilter(Json::Value const& _json)
{
    try
    {
        return toJS(client()->installWatch(toLogFilter(_json, *client())));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_newFilterEx(Json::Value const& _json)
{
    try
    {
        return toJS(client()->installWatch(toLogFilter(_json)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_newBlockFilter()
{
    h256 filter = dev::brc::ChainChangedFilter;
    return toJS(client()->installWatch(filter));
}

string Brc::brc_newPendingTransactionFilter()
{
    h256 filter = dev::brc::PendingChangedFilter;
    return toJS(client()->installWatch(filter));
}

bool Brc::brc_uninstallFilter(string const& _filterId)
{
    try
    {
        return client()->uninstallWatch(jsToInt(_filterId));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getFilterChanges(string const& _filterId)
{
    try
    {
        int id = jsToInt(_filterId);
        auto entries = client()->checkWatch(id);
        //		if (entries.size())
        //			cnote << "FIRING WATCH" << id << entries.size();
        return toJson(entries);
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getFilterChangesEx(string const& _filterId)
{
    try
    {
        int id = jsToInt(_filterId);
        auto entries = client()->checkWatch(id);
        //		if (entries.size())
        //			cnote << "FIRING WATCH" << id << entries.size();
        return toJsonByBlock(entries);
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getFilterLogs(string const& _filterId)
{
    try
    {
        return toJson(client()->logs(jsToInt(_filterId)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getFilterLogsEx(string const& _filterId)
{
    try
    {
        return toJsonByBlock(client()->logs(jsToInt(_filterId)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getLogs(Json::Value const& _json)
{
    try
    {
        return toJson(client()->logs(toLogFilter(_json, *client())));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getLogsEx(Json::Value const& _json)
{
    try
    {
        return toJsonByBlock(client()->logs(toLogFilter(_json)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getWork()
{
    try
    {
        Json::Value ret(Json::arrayValue);
        auto r = asBrchashClient(client())->getBrchashWork();
        ret.append(toJS(get<0>(r)));
        ret.append(toJS(get<1>(r)));
        ret.append(toJS(get<2>(r)));
        return ret;
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_syncing()
{
    dev::brc::SyncStatus sync = client()->syncStatus();
    if (sync.state == SyncState::Idle || !sync.majorSyncing)
        return Json::Value(false);

    Json::Value info(Json::objectValue);
    info["startingBlock"] = sync.startBlockNumber;
    info["highestBlock"] = sync.highestBlockNumber;
    info["currentBlock"] = sync.currentBlockNumber;
    return info;
}

string Brc::brc_chainId()
{
    return toJS(client()->chainId());
}

bool Brc::brc_submitWork(string const& _nonce, string const&, string const& _mixHash)
{
    try
    {
        return asBrchashClient(client())->submitBrchashWork(
            jsToFixed<32>(_mixHash), jsToFixed<Nonce::size>(_nonce));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

bool Brc::brc_submitHashrate(string const& _hashes, string const& _id)
{
    try
    {
        asBrchashClient(client())->submitExternalHashrate(jsToInt<32>(_hashes), jsToFixed<32>(_id));
        return true;
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_register(string const& _address)
{
    try
    {
        return toJS(m_brcAccounts.addProxyAccount(jsToAddress(_address)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

bool Brc::brc_unregister(string const& _accountId)
{
    try
    {
        return m_brcAccounts.removeProxyAccount(jsToInt(_accountId));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_fetchQueuedTransactions(string const& _accountId)
{
    try
    {
        auto id = jsToInt(_accountId);
        Json::Value ret(Json::arrayValue);
        // TODO: throw an error on no account with given id
        for (TransactionSkeleton const& t : m_brcAccounts.queuedTransactions(id))
            ret.append(toJson(t));
        m_brcAccounts.clearQueue(id);
        return ret;
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value dev::rpc::Brc::brc_getObtainVote(const std::string& _address, const std::string& _blockNumber)
{
	try
	{
		return client()->obtainVoteMessage(jsToAddress(_address), jsToBlockNumber(_blockNumber));
	}
	catch(...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}


Json::Value dev::rpc::Brc::brc_getVoted(const std::string& _address, const std::string& _blockNumber)
{
	try
	{
		return client()->votedMessage(jsToAddress(_address), jsToBlockNumber(_blockNumber));
	}
	catch(...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}


Json::Value dev::rpc::Brc::brc_getElector(const std::string& _blockNumber)
{
	try
	{
		return client()->electorMessage(jsToBlockNumber(_blockNumber));
	}
	catch(...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string dev::rpc::exceptionToErrorMessage()
{
    string ret;
    try
    {
        throw;
    }
    // Transaction submission exceptions
    catch (ZeroSignatureTransaction const&)
    {
        ret = "Zero signature transaction.";
    }
    catch (GasPriceTooLow const&)
    {
        ret = "Pending transaction with same nonce but higher gas price exists.";
    }
    catch (OutOfGasIntrinsic const&)
    {
        ret =
            "Transaction gas amount is less than the intrinsic gas amount for this transaction "
            "type.";
    }
    catch (BlockGasLimitReached const&)
    {
        ret = "Block gas limit reached.";
    }
    catch (InvalidNonce const&)
    {
        ret = "Invalid transaction nonce.";
    }
    catch (PendingTransactionAlreadyExists const&)
    {
        ret = "Same transaction already exists in the pending transaction queue.";
    }
    catch (TransactionAlreadyInChain const&)
    {
        ret = "Transaction is already in the blockchain.";
    }
    catch (NotEnoughCash const&)
    {
        ret = "Account balance is too low (balance < value + gas * gas price).";
    }
    catch (NotEnoughBallot const&)
    {
        ret = "Account balance is too low (balance < gas * gas price) or ballots is to low";
    }
    catch(VerifyVoteField const& ex)
	{
		ret = "Account vote verify is error!" + std::string(ex.what());
	}
	catch (InvalidSignature const&)
	{
		ret = "Invalid transaction signature.";
	}
	// Acount holder exceptions
	catch (AccountLocked const&)
	{
		ret = "Account is locked.";
	}
	catch (UnknownAccount const&)
	{
		ret = "Unknown account.";
	}
	catch (TransactionRefused const&)
	{
		ret = "Transaction rejected by user.";
	}
	catch (...)
	{
		ret = "Invalid RPC parameters.";
	}
	return ret;
}
