#include "NodePeer.hpp"

static const std::string CapbilityName = "Dpos";


namespace dev
{
namespace brc
{
void NodePeer::sendNewStatus(u256 height, h256 genesisHash, h256 latestBlock)
{
    CP2P_LOG << "sendNewStatus  height " << height << " genesisHash " << genesisHash
             << " latestBlock:" << latestBlock;
    RLPStream s;
    m_host->prep(m_id, CapbilityName, s, SHDposStatuspacket, 3)
        << height << genesisHash << latestBlock;
    m_host->sealAndSend(m_id, s);
}


}  // namespace brc
}  // namespace dev