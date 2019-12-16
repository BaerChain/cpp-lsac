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


#define MAXQUERIES 50
#define ZERONUM 0

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


string Brc::brc_gasPrice()
{
    return toJS(client()->GasAveragePrice());
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
    BOOST_THROW_EXCEPTION(JsonRpcException(std::string("This feature is not yet open")));
	try
	{
		return client()->successPendingOrderMessage(jsToInt(_getSize), jsToBlockNum(_blockNum));
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
            jsToAddress(_address), jsToInt(_getSize), jsToBlockNum(_blockNum));
	}
	catch (...)
	{
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Brc::brc_getPendingOrderPool(string const& _order_type, string const& _order_token_type,
    string const& _getSize, string const& _blockNumber)
{
    if(jsToU256(_getSize) <= 0 || jsToOrderEnum(_order_type) == 0)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(std::string("Unknown rpc parameter")));
    }
    try
    {
        return client()->pendingOrderPoolMessage(jsToOrderEnum(_order_type),
            1, jsToU256(_getSize), jsToBlockNum(_blockNumber));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getSuccessPendingOrderForAddr(string const& _address, string const& _minTime, string const& _maxTime, string const& _maxSize, string const& _blockNum)
{
    BOOST_THROW_EXCEPTION(JsonRpcException(std::string("This feature is not yet open")));
    if(jsToInt(_maxSize) > MAXQUERIES)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException("The number of query data cannot exceed 50"));
    }

    try {
        return client()->successPendingOrderForAddrMessage(jsToAddress(_address), jsToint64(_minTime),
                jsToint64(_maxTime), jsToInt(_maxSize), jsToBlockNum(_blockNum));
    }catch(...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }

}


Json::Value Brc::brc_getBalance(string const& _address, string const& _blockNumber)
{
    try
    {
        // return toJS(client()->balanceAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
        return client()->accountMessage(jsToAddress(_address), jsToBlockNum(_blockNumber));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getBlockReward(string const& _address, string const& _pageNum, string const& _listNum, string const& _blockNumber)
{
    try{
        if(jsToInt(_listNum) > 50)
        {
            BOOST_THROW_EXCEPTION(JsonRpcException(std::string("Entry size cannot exceed 50")));
        }
        if(jsToInt(_pageNum) < 0 || jsToInt(_listNum) < 0)
        {
            BOOST_THROW_EXCEPTION(JsonRpcException(std::string("Incoming parameters cannot be negative")));
        }
        return client()->blockRewardMessage(jsToAddress(_address), jsToInt(_pageNum), jsToInt(_listNum), jsToBlockNum(_blockNumber));
    }
    catch(...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getQueryExchangeReward(string const& _address, std::string const& _blockNumber)
{
    try {

        return client()->queryExchangeRewardMessage(jsToAddress(_address), jsToBlockNum(_blockNumber));
    }
    catch(...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getQueryBlockReward(string const& _address, std::string const& _blockNumber)
{

    try {
        return client()->queryBlockRewardMessage(jsToAddress(_address), jsToBlockNum(_blockNumber));
    }
    catch(...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

string Brc::brc_getBallot(string const& _address, string const& _blockNumber)
{
    try
    {
        return toJS(client()->ballotAt(jsToAddress(_address), jsToBlockNum(_blockNumber)));
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
                                                         jsToBlockNum(_blockNumber)),
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
            client()->stateRootAt(jsToAddress(_address), jsToBlockNum(_blockNumber)));
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
        return toJS(client()->countAt(jsToAddress(_address), jsToBlockNum(_blockNumber)));
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
        BlockNumber blockNumber = jsToBlockNum(_blockNumber);
        if (!client()->isKnown(blockNumber))
            return Json::Value(Json::nullValue);

        return toJS(client()->transactionCount(jsToBlockNum(_blockNumber)));
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
        BlockNumber blockNumber = jsToBlockNum(_blockNumber);
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
        return toJS(client()->codeAt(jsToAddress(_address), jsToBlockNum(_blockNumber)));
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
        throw JsonRpcException(exceptionToErrorMessage());
    }
}

std::string Brc::brc_importBlock(const std::string &_rlp) {
#ifndef NDEBUG
    try {
        auto by = jsToBytes(_rlp, OnFailed::Throw);
        return toJS(client()->importBlock(&by));
    }catch (const Exception &e){
        testlog << " error:" << e.what();
        throw JsonRpcException(exceptionToErrorMessage());
    }
#else
    return "this method only use debug.";
#endif
}

string Brc::brc_call(Json::Value const& _json, string const& _blockNumber)
{
    try
    {
        TransactionSkeleton t = toTransactionSkeleton(_json);
        setTransactionDefaults(t);
        ExecutionResult er = client()->call(t.from, t.value, t.to, t.data, t.gas, t.gasPrice,
                                            jsToBlockNum(_blockNumber), FudgeFactor::Lenient);
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

Json::Value Brc::brc_getBlockByHash(string const& _blockHash, bool _includeTransactions)
{
    try
    {
        h256 h = jsToFixed<32>(_blockHash);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactions(h), false, client()->sealEngine());
        else
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getBlockDetialByHash(const string &_blockHash, bool _includeTransactions)
{
    try
    {
        h256 h = jsToFixed<32>(_blockHash);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactions(h), true, client()->sealEngine());
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
        BlockNumber h = jsToBlockNum(_blockNumber);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactions(h), false, client()->sealEngine());
        else
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getBlockDetialByNumber(string const& _blockNumber, bool _includeTransactions)
{
    try
    {
        BlockNumber h = jsToBlockNum(_blockNumber);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactions(h), true, client()->sealEngine());
        else
            return toJson(client()->blockInfo(h), client()->blockDetails(h),
                client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getTransactionByHash(std::string const& _transactionHash)
{
    try
    {
        h256 h = jsToFixed<32>(_transactionHash);
        if (!client()->isKnownTransaction(h))
            return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransaction(h), false, client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getTransactionDetialByHash(std::string const& _transactionHash)
{
    try
    {
        h256 h = jsToFixed<32>(_transactionHash);
        if (!client()->isKnownTransaction(h))
            return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransaction(h), true, client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getAnalysisData(std::string const& _data)
{
    bytes r = fromHex(_data);
    return analysisData(r);
}

Json::Value Brc::brc_getTransactionByBlockHashAndIndex(
    string const& _blockHash, string const& _transactionIndex)
{
    try
    {
        h256 bh = jsToFixed<32>(_blockHash);
        unsigned ti = jsToInt(_transactionIndex);
        if (!client()->isKnownTransaction(bh, ti))
        {
            //return Json::Value(Json::nullValue);
            BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
        }

        return toJson(client()->localisedTransaction(bh, ti), false,client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getTransactionDetialByBlockHashAndIndex(
        string const& _blockHash, string const& _transactionIndex)
{
    try
    {
        h256 bh = jsToFixed<32>(_blockHash);
        unsigned ti = jsToInt(_transactionIndex);
        if (!client()->isKnownTransaction(bh, ti))
            return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransaction(bh, ti), true, client()->sealEngine());
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
        BlockNumber bn = jsToBlockNum(_blockNumber);
        if (!client()->isKnown(bn))
        {
            BOOST_THROW_EXCEPTION(JsonRpcException(std::string("Unknown block height")));
        }
            //return Json::Value(Json::nullValue);
        h256 bh = client()->hashFromNumber(bn);
        unsigned ti = jsToInt(_transactionIndex);
        if (!client()->isKnownTransaction(bh, ti))
        {
            BOOST_THROW_EXCEPTION(JsonRpcException(std::string("Unknown trading index")));
        }
            //return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransaction(bh, ti), false, client()->sealEngine());
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Brc::brc_getTransactionDetialByBlockNumberAndIndex(
        string const& _blockNumber, string const& _transactionIndex)
{
    try
    {
        BlockNumber bn = jsToBlockNum(_blockNumber);
        h256 bh = client()->hashFromNumber(bn);
        unsigned ti = jsToInt(_transactionIndex);
        if (!client()->isKnownTransaction(bh, ti))
            return Json::Value(Json::nullValue);

        return toJson(client()->localisedTransaction(bh, ti), false,client()->sealEngine());
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
        BlockNumber h = jsToBlockNum(_blockNumber);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);
        return toJson(client()->uncle(jsToBlockNum(_blockNumber), jsToInt(_uncleIndex)),
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
		return client()->obtainVoteMessage(jsToAddress(_address), jsToBlockNum(_blockNumber));
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
		return client()->votedMessage(jsToAddress(_address), jsToBlockNum(_blockNumber));
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
		return client()->electorMessage(jsToBlockNum(_blockNumber));
	}
	catch(...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value dev::rpc::Brc::brc_estimateGasUsed(const Json::Value &_json)
{
    try
    {
        return client()->estimateGasUsed(_json, PendingBlock);
    }
    catch(EstimateGasUsed const& _e)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(std::string(*boost::get_error_info<errinfo_comment >(_e))));
    }
    catch(...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
    
}

Json::Value dev::rpc::Brc::brc_orderMessage(const std::string& key, const std::string& _blockNumber){
    try
    {
        return client()->testBplusGet(dev::jsToU256(key), jsToBlockNum(_blockNumber));
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
    catch (InvalidNonce const& ex)
    {
        ret = "Invalid transaction nonce.";
		if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
			ret += std::string(*_error);
    }
	catch(InvalidFunction const& ex){
		ret = "Invalid function .";
		if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
			ret += std::string(*_error);
	}
    catch (PendingTransactionAlreadyExists const&)
    {
        ret = "Same transaction already exists in the pending transaction queue.";
    }
    catch (TransactionAlreadyInChain const&)
    {
        ret = "Transaction is already in the blockchain.";
    }
    catch (NotEnoughCash const& ex)
    {
        ret = "Account balance is too low (balance < value + gas * gas price)." ;
		if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
		   ret += std::string(*_error);
    }
    catch (VerifyPendingOrderFiled const& _v)
    {
        ret = "verifyPendingorder failed :" + std::string(*boost::get_error_info<errinfo_comment>(_v));
    }
    catch (CancelPendingOrderFiled const& _c)
    {
        ret = "cancelPendingorder failed :" + std::string(*boost::get_error_info<errinfo_comment>(_c));
    }
    catch (receivingincomeFiled const& _r)
    {
        ret = "receivingincome failed :" + std::string(*boost::get_error_info<errinfo_comment >(_r));
    }
    catch (transferAutoExFailed const& _t)
    {
        ret = "transferAutoEx failed: " + std::string(*boost::get_error_info<errinfo_comment>(_t));
    }
    catch (getVotingCycleFailed const _g)
    {
        ret = std::string(*boost::get_error_info<errinfo_comment >(_g));
    }
    catch (NotEnoughBallot const&)
    {
        ret = "Account balance is too low (balance < gas * gas price) or ballots is to low";
    }
    catch(VerifyVoteField const& ex)
	{
		ret = "Account vote verify is error :";
		if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
			ret += std::string(*_error);
	}
    catch(InvalidGasPrice const& ex)
	{
		ret = "Accoun's transaction is error :";
		if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
			ret += std::string(*_error);
	}
	catch(BrcTranscationField const& ex)
	{
		ret = "Account transfer BRC verify is error :";
		if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
			ret += std::string(*_error);
	}
	catch (InvalidSignature const&ex)
	{
		ret = "Invalid transaction signature.";
		if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
			ret += std::string(*_error);
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
	catch (ChangeMinerFailed const &e){
        ret = "Replace witness operations cannot be batch operated.";
	}
    catch (InvalidAutor const&ex)
    {
        ret = "";
        if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
            ret += std::string(*_error);
    }
    catch (ExecutiveFailed const& _e)
    {
        if(auto *_error = boost::get_error_info<errinfo_comment>(_e))
        {
            ret += std::string(*_error);
        }
    }
	catch (...)
	{
		ret = "Invalid RPC parameters.";
	}
	return ret;
}
