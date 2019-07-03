#include "Test.h"
#include <jsonrpccpp/common/errors.h>
#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonJS.h>
#include <libbrcdchain/ChainParams.h>
#include <libbrcdchain/ClientTest.h>

using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace dev::rpc;
using namespace jsonrpc;

Test::Test(brc::Client& _brc): m_brc(_brc) {}

namespace
{
string logEntriesToLogHash(brc::LogEntries const& _logs)
{
    RLPStream s;
    s.appendList(_logs.size());
    for (brc::LogEntry const& l : _logs)
        l.streamRLP(s);
    return toJS(sha3(s.out()));
}

h256 stringToHash(string const& _hashString)
{
    try
    {
        return h256(_hashString);
    }
    catch (BadHexCharacter const&)
    {
        throw JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS);
    }
}
}

string Test::test_getLogHash(string const& _txHash)
{
    try
    {
        h256 txHash = stringToHash(_txHash);
        if (m_brc.blockChain().isKnownTransaction(txHash))
        {
            LocalisedTransaction t = m_brc.localisedTransaction(txHash);
            BlockReceipts const& blockReceipts = m_brc.blockChain().receipts(t.blockHash());
            if (blockReceipts.receipts.size() != 0)
                return logEntriesToLogHash(blockReceipts.receipts[t.transactionIndex()].log());
        }
        return toJS(dev::EmptyListSHA3);
    }
    catch (std::exception const& ex)
    {
        cwarn << ex.what();
        throw JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR, ex.what());
    }
}

bool Test::test_setChainParams(Json::Value const& param1)
{
    try
    {
        Json::FastWriter fastWriter;
        std::string output = fastWriter.write(param1);
        asClientTest(m_brc).setChainParams(output);
        asClientTest(m_brc).completeSync();  // set sync state to idle for mining
        return true;
    }
    catch (std::exception const& ex)
    {
        cwarn << ex.what();
        throw JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR, ex.what());
    }
}

bool Test::test_mineBlocks(int _number)
{
    if (!asClientTest(m_brc).mineBlocks(_number))  // Synchronous
        throw JsonRpcException("Mining failed.");
    return true;
}

bool Test::test_modifyTimestamp(int _timestamp)
{
    // FIXME: Fix year 2038 issue.
    try
    {
        asClientTest(m_brc).modifyTimestamp(_timestamp);
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }
    return true;
}

bool Test::test_rewindToBlock(int _number)
{
    try
    {
        m_brc.rewind(_number);
        asClientTest(m_brc).completeSync();
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }
    return true;
}

std::string Test::test_importRawBlock(string const& _blockRLP)
{
    try
    {
        ClientTest& client = asClientTest(m_brc);
        return toJS(client.importRawBlock(_blockRLP));
    }
    catch (std::exception const& ex)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR, ex.what()));
    }
}
