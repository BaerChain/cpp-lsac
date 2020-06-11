#include "NodePeer.hpp"

static const std::string CapbilityName = "Dpos";


namespace dev
{
namespace brc
{
void NodePeer::sendNewStatus(u256 height, h256 genesisHash, h256 latestBlock, uint32_t version)
{
    CP2P_LOG << "sendNewStatus  height " << height << " genesisHash " << genesisHash
             << " latestBlock:" << latestBlock;
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposStatuspacket, 4)
        << height << genesisHash << latestBlock << version;
    m_host->sealAndSend(m_id, s);
}


void NodePeer::setPeerStatus(u256 height, h256 genesisHash, h256 latestBlock, uint32_t version)
{
    m_height = (uint64_t)height;
    m_genesisHash = genesisHash;
    m_latestBlock = latestBlock;
    m_version = version;
}

void NodePeer::requestNodeConfig(const std::vector<uint64_t>& ids)
{
    if (checkRequest())
    {
        return;
    }
    m_ask = SHposAsking::Request;
    CP2P_LOG << "requestNodeConfig  by number " << ids.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDpodUpNodeConfig, 2);
    s.append(1);
    s.appendVector(ids);
    m_host->sealAndSend(m_id, s);
}
void NodePeer::sendConfigBlocks(const std::vector<bytes>& blocks){
    CP2P_LOG << "send  config blocks   " << blocks.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDpodNodeConfig, 1);
    s.appendVector(blocks);
    m_host->sealAndSend(m_id, s);
}

void NodePeer::requestBlocks(const std::vector<uint64_t>& ids)
{
    if (checkRequest())
    {
        return;
    }
    m_ask = SHposAsking::Request;
    CP2P_LOG << "requestBlocks  by number " << ids.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposSyncGetBlocks, 2);
    s.append(1);
    s.appendVector(ids);
    m_host->sealAndSend(m_id, s);
}


void NodePeer::requestBlocks(const std::vector<h256>& ids)
{
    if (checkRequest())
    {
        return;
    }
    m_ask = SHposAsking::Request;
    CP2P_LOG << "requestBlocks  by hash " << ids;
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposSyncGetBlocks, 2);
    s.append(2);
    s.appendVector(ids);
    m_host->sealAndSend(m_id, s);
}

void NodePeer::sendBlocks(const std::vector<bytes>& blocks)
{
    CP2P_LOG << "send blocks   " << blocks.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposSyncBlockHeaders, 1);
    s.appendVector(blocks);
    m_host->sealAndSend(m_id, s);
}

void NodePeer::sendTransactionHashs(const std::vector<h256>& hashs)
{
    CP2P_LOG << "send tx hash " << hashs.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposBroadcastTXHash, 1);
    s.appendVector(hashs);
    CP2P_LOG << toJS(s.out());
    m_host->sealAndSend(m_id, s);
}

void NodePeer::sendTransactionBody(const std::vector<bytes>& data)
{
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposGetTransactions, 1);
    s.appendVector(data);
    m_host->sealAndSend(m_id, s);
}

void NodePeer::requestTxs(const std::vector<h256>& hashs)
{
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposGetTransactionsForRequest, 1);
    s.appendVector(hashs);
    m_host->sealAndSend(m_id, s);
}

void NodePeer::sendBlocksHashs(const std::vector<h256>& hashs)
{
    std::vector<h256> sends;
    for(auto const& h : hashs){
        sends.emplace_back(h);
    }
    if (sends.empty())
        return;
    //CP2P_LOG << "send blocks hash " << hashs;
    //CP2P_LOG << "send blocks:" << hashs.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposNewBlockHash, 1);
    s.appendVector(hashs);
    m_host->sealAndSend(m_id, s);
}


void NodePeer::requestState()
{
    CP2P_LOG << "request " << __FUNCTION__ ;
    if (checkRequest())
    {
        return;
    }
    m_ask = SHposAsking::Request;
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposGetState, 0);
    m_host->sealAndSend(m_id, s);
}

void NodePeer::sendState(u256 height, h256 genesisHash, h256 latestBlock, uint32_t version)
{
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposUpdateState, 4)
        << height << genesisHash << latestBlock << version;
    m_host->sealAndSend(m_id, s);
}


uint64_t NodePeer::getHeight() const
{
    return m_height;
}
h256 NodePeer::getLatestBlock() const
{
    return m_latestBlock;
}
uint32_t NodePeer::getVersion() const
{
    return m_version;
}

bool NodePeer::checkRequest() const
{
    if (SHposAsking::Request == m_ask)
    {
        CP2P_LOG << "can't requeset, this peer is request";
        return true;
    }
    return false;
}


}  // namespace brc
}  // namespace dev