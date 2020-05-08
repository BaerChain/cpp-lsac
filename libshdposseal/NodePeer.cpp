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


void NodePeer::requestBlocks(const std::vector<uint64_t>& ids)
{
    CP2P_LOG << "requestBlocks  by number " << ids.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposSyncGetBlocks, 2);
    s.append(1);
    s.appendVector(ids);
    m_host->sealAndSend(m_id, s);
}


void NodePeer::requestBlocks(const std::vector<h256>& ids)
{
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
    m_host->prep(m_id, CapbilityName, s, SHDposTXHash, 1);
    s.appendVector(hashs);
    m_host->sealAndSend(m_id, s);
}

void NodePeer::sendBlocksHashs(const std::vector<h256>& hashs)
{
    CP2P_LOG << "send blocks hash " << hashs;
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposNewBlockHash, 1);
    s.appendVector(hashs);
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


}  // namespace brc
}  // namespace dev