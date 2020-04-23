#include "SHDposHostCapability.h"


namespace dev
{
namespace brc
{
SHDposHostcapability::SHDposHostcapability(std::shared_ptr<p2p::CapabilityHostFace> _host,
    BlockChain const& _ch, OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq,
    u256 _networkId)
  : m_host(std::move(_host))
  ,m_chain(_ch)
  ,m_db(_db)
  ,m_tq(_tq)
  ,m_bq(_bq)
  ,m_networkId(_networkId)

{}

void SHDposHostcapability::onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion)
{
    CP2P_LOG << "connect new capability";
    NodePeer  np(m_host, _nodeID);
    m_peers.emplace(_nodeID, np);

    m_peers[_nodeID].sendNewStatus(m_chain.details().number, m_chain.genesisHash(), m_chain.currentHash());
}   

bool SHDposHostcapability::interpretCapabilityPacket(
    NodeID const& _peerID, unsigned _id, RLP const& _r)
{
    CP2P_LOG << "get new message from interpretCapabilityPacket";
    try
    {
        switch (_id){
            case SHDposStatuspacket:{
                CP2P_LOG << "SHDposStatuspacket ";
                break;
            }
            default:{
                CP2P_LOG << "cant resolve protocol.";
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    


    return true;
}


}  // namespace brc
}  // namespace dev
