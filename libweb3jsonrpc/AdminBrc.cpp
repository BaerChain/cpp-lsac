#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonJS.h>
#include <libbrccore/KeyManager.h>
#include <libbrcdchain/Client.h>
#include <libbrcdchain/Executive.h>
#include <libbrchashseal/BrchashClient.h>
#include "AdminBrc.h"
#include "SessionManager.h"
#include "JsonHelper.h"
using namespace std;
using namespace dev;
using namespace dev::rpc;
using namespace dev::brc;

AdminBrc::AdminBrc(brc::Client& _brc, brc::TrivialGasPricer& _gp, brc::KeyManager& _keyManager, SessionManager& _sm):
    m_brc(_brc),
    m_gp(_gp),
    m_keyManager(_keyManager),
    m_sm(_sm)
{}

bool AdminBrc::admin_brc_setMining(bool _on, string const& _session)
{
    RPC_ADMIN;
    if (_on)
        m_brc.startSealing();
    else
        m_brc.stopSealing();
    return true;
}

Json::Value AdminBrc::admin_brc_blockQueueStatus(string const& _session)
{
    RPC_ADMIN;
    Json::Value ret;
    BlockQueueStatus bqs = m_brc.blockQueue().status();
    ret["importing"] = (int)bqs.importing;
    ret["verified"] = (int)bqs.verified;
    ret["verifying"] = (int)bqs.verifying;
    ret["unverified"] = (int)bqs.unverified;
    ret["future"] = (int)bqs.future;
    ret["unknown"] = (int)bqs.unknown;
    ret["bad"] = (int)bqs.bad;
    return ret;
}

bool AdminBrc::admin_brc_setAskPrice(string const& _wei, string const& _session)
{
    RPC_ADMIN;
    m_gp.setAsk(jsToU256(_wei));
    return true;
}

bool AdminBrc::admin_brc_setBidPrice(string const& _wei, string const& _session)
{
    RPC_ADMIN;
    m_gp.setBid(jsToU256(_wei));
    return true;
}

Json::Value AdminBrc::admin_brc_findBlock(string const& _blockHash, string const& _session)
{
    RPC_ADMIN;
    h256 h(_blockHash);
    if (m_brc.blockChain().isKnown(h))
        return toJson(m_brc.blockChain().info(h));
    switch(m_brc.blockQueue().blockStatus(h))
    {
        case QueueStatus::Ready:
            return "ready";
        case QueueStatus::Importing:
            return "importing";
        case QueueStatus::UnknownParent:
            return "unknown parent";
        case QueueStatus::Bad:
            return "bad";
        default:
            return "unknown";
    }
}

string AdminBrc::admin_brc_blockQueueFirstUnknown(string const& _session)
{
    RPC_ADMIN;
    return m_brc.blockQueue().firstUnknown().hex();
}

bool AdminBrc::admin_brc_blockQueueRetryUnknown(string const& _session)
{
    RPC_ADMIN;
    m_brc.retryUnknown();
    return true;
}

Json::Value AdminBrc::admin_brc_allAccounts(string const& _session)
{
    RPC_ADMIN;
    Json::Value ret;
    u256 total = 0;
    u256 pendingtotal = 0;
    Address beneficiary;
    for (auto const& address: m_keyManager.accounts())
    {
        auto pending = m_brc.balanceAt(address, PendingBlock);
        auto latest = m_brc.balanceAt(address, LatestBlock);
        Json::Value a;
        if (address == beneficiary)
            a["beneficiary"] = true;
        a["address"] = toJS(address);
        a["balance"] = toJS(latest);
        a["nicebalance"] = formatBalance(latest);
        a["pending"] = toJS(pending);
        a["nicepending"] = formatBalance(pending);
        ret["accounts"][m_keyManager.accountName(address)] = a;
        total += latest;
        pendingtotal += pending;
    }
    ret["total"] = toJS(total);
    ret["nicetotal"] = formatBalance(total);
    ret["pendingtotal"] = toJS(pendingtotal);
    ret["nicependingtotal"] = formatBalance(pendingtotal);
    return ret;
}

Json::Value AdminBrc::admin_brc_newAccount(Json::Value const& _info, string const& _session)
{
    RPC_ADMIN;
    if (!_info.isMember("name"))
        throw jsonrpc::JsonRpcException("No member found: name");
    string name = _info["name"].asString();
    KeyPair kp = KeyPair::create();
    h128 uuid;
    if (_info.isMember("password"))
    {
        string password = _info["password"].asString();
        string hint = _info["passwordHint"].asString();
        uuid = m_keyManager.import(kp.secret(), name, password, hint);
    }
    else
        uuid = m_keyManager.import(kp.secret(), name);
    Json::Value ret;
    ret["account"] = toJS(kp.pub());
    ret["uuid"] = toUUID(uuid);
    return ret;
}

bool AdminBrc::admin_brc_setMiningBenefactor(string const& _uuidOrAddress, string const& _session)
{
    RPC_ADMIN;
    return miner_setBrcerbase(_uuidOrAddress);
}

Json::Value AdminBrc::admin_brc_inspect(string const& _address, string const& _session)
{
    RPC_ADMIN;
    if (!isHash<Address>(_address))
        throw jsonrpc::JsonRpcException("Invalid address given.");
    
    Json::Value ret;
    auto h = Address(fromHex(_address));
    ret["storage"] = toJson(m_brc.storageAt(h, PendingBlock));
    ret["balance"] = toJS(m_brc.balanceAt(h, PendingBlock));
    ret["nonce"] = toJS(m_brc.countAt(h, PendingBlock));
    ret["code"] = toJS(m_brc.codeAt(h, PendingBlock));
    return ret;
}

h256 AdminBrc::blockHash(string const& _blockNumberOrHash) const
{
    if (isHash<h256>(_blockNumberOrHash))
        return h256(_blockNumberOrHash.substr(_blockNumberOrHash.size() - 64, 64));
    try
    {
        return m_brc.blockChain().numberHash(stoul(_blockNumberOrHash));
    }
    catch (...)
    {
        throw jsonrpc::JsonRpcException("Invalid argument");
    }
}

Json::Value AdminBrc::admin_brc_reprocess(string const& _blockNumberOrHash, string const& _session)
{
    RPC_ADMIN;
    Json::Value ret;
    PopulationStatistics ps;
    m_brc.block(blockHash(_blockNumberOrHash), &ps);
    ret["enact"] = ps.enact;
    ret["verify"] = ps.verify;
    ret["total"] = ps.verify + ps.enact;
    return ret;
}

Json::Value AdminBrc::admin_brc_vmTrace(string const& _blockNumberOrHash, int _txIndex, string const& _session)
{
    RPC_ADMIN;
    
    Json::Value ret;
    
    if (_txIndex < 0)
        throw jsonrpc::JsonRpcException("Negative index");
    Block block = m_brc.block(blockHash(_blockNumberOrHash));
    if ((unsigned)_txIndex < block.pending().size())
    {
        Transaction t = block.pending()[_txIndex];
        State s(State::Null);
        Executive e(s, block, _txIndex, m_brc.blockChain());
        try
        {
            StandardTrace st;
            st.setShowMnemonics();
            e.initialize(t);
            if (!e.execute())
                e.go(st.onOp());
            e.finalize();
            ret["structLogs"] = st.jsonValue();
        }
        catch(Exception const& _e)
        {
            cwarn << diagnostic_information(_e);
        }
    }
    
    return ret;
}

Json::Value AdminBrc::admin_brc_getReceiptByHashAndIndex(string const& _blockNumberOrHash, int _txIndex, string const& _session)
{
    RPC_ADMIN;
    if (_txIndex < 0)
        throw jsonrpc::JsonRpcException("Negative index");
    auto h = blockHash(_blockNumberOrHash);
    if (!m_brc.blockChain().isKnown(h))
        throw jsonrpc::JsonRpcException("Invalid/unknown block.");
    auto rs = m_brc.blockChain().receipts(h);
    if ((unsigned)_txIndex >= rs.receipts.size())
        throw jsonrpc::JsonRpcException("Index too large.");
    return toJson(rs.receipts[_txIndex]);
}

bool AdminBrc::miner_start(int)
{
    m_brc.startSealing();
    return true;
}

bool AdminBrc::miner_stop()
{
    m_brc.stopSealing();
    return true;
}

bool AdminBrc::miner_setBrcerbase(string const& _uuidOrAddress)
{
    Address a;
    h128 uuid = fromUUID(_uuidOrAddress);
    if (uuid)
        a = m_keyManager.address(uuid);
    else if (isHash<Address>(_uuidOrAddress))
        a = Address(_uuidOrAddress);
    else
        throw jsonrpc::JsonRpcException("Invalid UUID or address");

    if (m_setMiningBenefactor)
        m_setMiningBenefactor(a);
    else
        m_brc.setAuthor(a);
    return true;
}

bool AdminBrc::miner_setExtra(string const& _extraData)
{
    m_brc.setExtraData(asBytes(_extraData));
    return true;
}

bool AdminBrc::miner_setGasPrice(string const& _gasPrice)
{
    m_gp.setAsk(jsToU256(_gasPrice));
    return true;
}

string AdminBrc::miner_hashrate()
{
    BrchashClient const* client = nullptr;
    try
    {
        client = asBrchashClient(&m_brc);
    }
    catch (...)
    {
        throw jsonrpc::JsonRpcException("Hashrate not available - blockchain does not support mining.");
    }
    return toJS(client->hashrate());
}
