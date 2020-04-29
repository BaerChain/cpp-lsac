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


void NodePeer::setPeerStatus(u256 height, h256 genesisHash, h256 latestBlock, uint32_t version) {}


void NodePeer::requestBlocks(const std::vector<uint64_t>& ids)
{
    CP2P_LOG << "requestBlocks  " << ids.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposRequestBlocks, 2);
    s.append(1);
    s.appendVector(ids);
    m_host->sealAndSend(m_id, s);
}


void NodePeer::requestBlocks(const std::vector<h256>& ids)
{
    CP2P_LOG << "requestBlocks  " << ids.size();
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposRequestBlocks, 2);
    s.append(2);
    s.appendVector(ids);
    m_host->sealAndSend(m_id, s);
}

void NodePeer::sendBlocks(const std::vector<BlockHeader>& blocks) {
    
}

}  // namespace brc
}  // namespace dev