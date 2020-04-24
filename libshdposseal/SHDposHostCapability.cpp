#include "SHDposHostCapability.h"


namespace dev
{
namespace brc
{
SHDposHostcapability::SHDposHostcapability(std::shared_ptr<p2p::CapabilityHostFace> _host,
    BlockChain const& _ch, OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq,
    u256 _networkId, uint32_t version)
  : m_host(std::move(_host))
  ,m_chain(_ch)
  ,m_db(_db)
  ,m_tq(_tq)
  ,m_bq(_bq)
  ,m_networkId(_networkId)
  ,m_version(version)
  ,m_sync(nullptr)

{
    
}

void SHDposHostcapability::onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion)
{
    CP2P_LOG << "connect new capability";
    NodePeer  np(m_host, _nodeID);
    m_peers.emplace(_nodeID, np);
    auto header = m_chain.info();
    m_peers[_nodeID].sendNewStatus(header.number(), m_chain.genesisHash(), header.hash(), m_version);
}   

bool SHDposHostcapability::interpretCapabilityPacket(
    NodeID const& _peerID, unsigned _id, RLP const& _r)
{
    
    CP2P_LOG << "get new message from interpretCapabilityPacket " << std::this_thread::get_id();
    auto& peer = m_peers[_peerID];
    try
    {
        switch (_id){
            case SHDposStatuspacket:{
                CP2P_LOG << "SHDposStatuspacket";
                
                auto number = _r[0].toInt<u256>();
                auto genesisHash = _r[1].toHash<h256>();
                auto hash = _r[2].toHash<h256>();
                peer.setPeerStatus(number, genesisHash, hash);

                //TODO: sync.
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
