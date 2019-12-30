#include "BrcV2.h"
#include "AccountHolder.h"
#include <jsonrpccpp/common/exception.h>
#include <libbrccore/CommonJS.h>
#include <libbrcdchain/Client.h>
#include <libbrchashseal/BrchashClient.h>
#include <libdevcore/CommonData.h>
#include <libweb3jsonrpc/JsonHelperV2.h>
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

#define CATCHRPCEXCEPTION catch (Exception const& ex) { throw JsonRpcException(exceptionToErrorMessage());}  \
                          catch(...){ BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));}

BrcV2::BrcV2(brc::Interface& _brc, brc::AccountHolder& _brcAccounts)
        : m_brc(_brc), m_brcAccounts(_brcAccounts)
{}

Json::Value BrcV2::brc_getPendingOrderPoolForAddr(
        string const& _address, string const& _getSize, string const& _blockNum)
{
    try
    {
        return client()->pendingOrderPoolForAddrMessage(
                jsToAddressFromNewAddress(_address), jsToInt(_getSize), jsToBlockNum(_blockNum));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getPendingOrderPool(string const& _order_type, string const& _order_token_type,
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
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getBalance(string const& _address, string const& _blockNumber)
{
    try
    {
        // return toJS(client()->balanceAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
        return client()->accountMessage(jsToAddressFromNewAddress(_address), jsToBlockNum(_blockNumber));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getQueryExchangeReward(string const& _address, std::string const& _blockNumber)
{
    try {

        return client()->queryExchangeRewardMessage(jsToAddressFromNewAddress(_address), jsToBlockNum(_blockNumber));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getQueryBlockReward(string const& _address, std::string const& _blockNumber)
{

    try {
        return client()->queryBlockRewardMessage(jsToAddressFromNewAddress(_address), jsToBlockNum(_blockNumber));
    }
    CATCHRPCEXCEPTION
}

string BrcV2::brc_getBallot(string const& _address, string const& _blockNumber)
{
    try
    {
        return toJS(client()->ballotAt(jsToAddressFromNewAddress(_address), jsToBlockNum(_blockNumber)));
    }
    CATCHRPCEXCEPTION
}

string BrcV2::brc_getStorageAt(
        string const& _address, string const& _position, string const& _blockNumber)
{
    try
    {
        return toJS(toCompactBigEndian(client()->stateAt(jsToAddressFromNewAddress(_address), jsToU256(_position),
                                                         jsToBlockNum(_blockNumber)), 32));
    }
    CATCHRPCEXCEPTION
}

string BrcV2::brc_getStorageRoot(string const& _address, string const& _blockNumber)
{
    try
    {
        return toString(
                client()->stateRootAt(jsToAddressFromNewAddress(_address), jsToBlockNum(_blockNumber)));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_pendingTransactions()
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

    return toJsonV2(ours);
}

string BrcV2::brc_getTransactionCount(string const& _address, string const& _blockNumber)
{
    try
    {
        return toJS(client()->countAt(jsToAddressFromNewAddress(_address), jsToBlockNum(_blockNumber)));
    }
    CATCHRPCEXCEPTION
}

void BrcV2::setTransactionDefaults(TransactionSkeleton& _t)
{
    if (!_t.from)
        _t.from = m_brcAccounts.defaultTransactAccount();
}

Json::Value BrcV2::brc_inspectTransaction(std::string const& _rlp)
{
    try
    {
        return toJsonV2(Transaction(jsToBytes(_rlp, OnFailed::Throw), CheckTransaction::Everything));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getBlockByHash(string const& _blockHash, bool _includeTransactions)
{
    try
    {
        h256 h = jsToFixed<32>(_blockHash);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJsonV2(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactions(h), false, client()->sealEngine());
        else
            return toJsonV2(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getBlockDetialByHash(const string &_blockHash, bool _includeTransactions)
{
    try
    {
        h256 h = jsToFixed<32>(_blockHash);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJsonV2(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactions(h), true, client()->sealEngine());
        else
            return toJsonV2(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}



Json::Value BrcV2::brc_getBlockByNumber(string const& _blockNumber, bool _includeTransactions)
{
    try
    {
        BlockNumber h = jsToBlockNum(_blockNumber);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJsonV2(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactions(h), false, client()->sealEngine());
        else
            return toJsonV2(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getBlockDetialByNumber(string const& _blockNumber, bool _includeTransactions)
{
    try
    {
        BlockNumber h = jsToBlockNum(_blockNumber);
        if (!client()->isKnown(h))
            return Json::Value(Json::nullValue);

        if (_includeTransactions)
            return toJsonV2(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactions(h), true, client()->sealEngine());
        else
            return toJsonV2(client()->blockInfo(h), client()->blockDetails(h),
                          client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getTransactionByHash(std::string const& _transactionHash)
{
    try
    {
        h256 h = jsToFixed<32>(_transactionHash);
        if (!client()->isKnownTransaction(h))
            return Json::Value(Json::nullValue);

        return toJsonV2(client()->localisedTransaction(h), false, client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getTransactionDetialByHash(std::string const& _transactionHash)
{
    try
    {
        h256 h = jsToFixed<32>(_transactionHash);
        if (!client()->isKnownTransaction(h))
            return Json::Value(Json::nullValue);

        return toJsonV2(client()->localisedTransaction(h), true, client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getAnalysisData(std::string const& _data)
{
    bytes r = fromHex(_data);
    try {
        return analysisDataV2(r);
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getTransactionByBlockHashAndIndex(
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

        return toJsonV2(client()->localisedTransaction(bh, ti), false,client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getTransactionDetialByBlockHashAndIndex(
        string const& _blockHash, string const& _transactionIndex)
{
    try
    {
        h256 bh = jsToFixed<32>(_blockHash);
        unsigned ti = jsToInt(_transactionIndex);
        if (!client()->isKnownTransaction(bh, ti))
            return Json::Value(Json::nullValue);

        return toJsonV2(client()->localisedTransaction(bh, ti), true, client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}



Json::Value BrcV2::brc_getTransactionByBlockNumberAndIndex(
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

        return toJsonV2(client()->localisedTransaction(bh, ti), false, client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getTransactionDetialByBlockNumberAndIndex(
        string const& _blockNumber, string const& _transactionIndex)
{
    try
    {
        BlockNumber bn = jsToBlockNum(_blockNumber);
        h256 bh = client()->hashFromNumber(bn);
        unsigned ti = jsToInt(_transactionIndex);
        if (!client()->isKnownTransaction(bh, ti))
            return Json::Value(Json::nullValue);

        return toJsonV2(client()->localisedTransaction(bh, ti), false,client()->sealEngine());
    }
    CATCHRPCEXCEPTION
}


Json::Value BrcV2::brc_getTransactionReceipt(string const& _transactionHash)
{
    try
    {
        h256 h = jsToFixed<32>(_transactionHash);
        if (!client()->isKnownTransaction(h))
            return Json::Value(Json::nullValue);

        return toJsonV2(client()->localisedTransactionReceipt(h));
    }
    CATCHRPCEXCEPTION
}

string BrcV2::brc_newPendingTransactionFilter()
{
    h256 filter = dev::brc::PendingChangedFilter;
    return toJS(client()->installWatch(filter));
}


Json::Value BrcV2::brc_getFilterChangesEx(string const& _filterId)
{
    try
    {
        int id = jsToInt(_filterId);
        auto entries = client()->checkWatch(id);
        //		if (entries.size())
        //			cnote << "FIRING WATCH" << id << entries.size();
        return toJsonV2ByBlock(entries);
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getFilterLogs(string const& _filterId)
{
    try
    {
        return toJsonV2(client()->logs(jsToInt(_filterId)));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getFilterLogsEx(string const& _filterId)
{
    try
    {
        return toJsonV2ByBlock(client()->logs(jsToInt(_filterId)));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getLogs(Json::Value const& _json)
{
    try
    {
        return toJsonV2(client()->logs(toLogFilterV2(_json, *client())));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_getLogsEx(Json::Value const& _json)
{
    try
    {
        return toJsonV2ByBlock(client()->logs(toLogFilterV2(_json)));
    }
    CATCHRPCEXCEPTION
}

Json::Value BrcV2::brc_fetchQueuedTransactions(string const& _accountId)
{
    try
    {
        auto id = jsToInt(_accountId);
        Json::Value ret(Json::arrayValue);
        // TODO: throw an error on no account with given id
        for (TransactionSkeleton const& t : m_brcAccounts.queuedTransactions(id))
            ret.append(toJsonV2(t));
        m_brcAccounts.clearQueue(id);
        return ret;
    }
    CATCHRPCEXCEPTION
}

Json::Value dev::rpc::BrcV2::brc_getObtainVote(const std::string& _address, const std::string& _blockNumber)
{
    try
    {
        return client()->obtainVoteMessage(jsToAddressFromNewAddress(_address), jsToBlockNum(_blockNumber));
    }
    CATCHRPCEXCEPTION
}


Json::Value dev::rpc::BrcV2::brc_getVoted(const std::string& _address, const std::string& _blockNumber)
{
    try
    {
        auto  address = jsToAddressFromNewAddress(_address);
        auto ret= client()->votedMessage(address, jsToBlockNum(_blockNumber));
        return ret;
    }
    CATCHRPCEXCEPTION
}


Json::Value dev::rpc::BrcV2::brc_getElector(const std::string& _blockNumber)
{
    try
    {
        return client()->electorMessage(jsToBlockNum(_blockNumber));
    }
    CATCHRPCEXCEPTION
}

Json::Value dev::rpc::BrcV2::brc_estimateGasUsed(const Json::Value &_json)
{
    try
    {
        return client()->estimateGasUsed(_json, PendingBlock);
    }
    catch(EstimateGasUsed const& _e)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(std::string(*boost::get_error_info<errinfo_comment >(_e))));
    }
    CATCHRPCEXCEPTION

}

/*
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
    catch (ChangeMinerFailed const &ex){
        ret = "ChangeMinerFailed : ";
        if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
            ret += std::string(*_error);
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

*/