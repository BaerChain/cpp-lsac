#pragma once
#include "CommonNet.h"
#include "Common.h"
#include <libp2p/CapabilityHost.h>
namespace dev
{
namespace brc
{

    
class NodePeer
{
public:
    NodePeer(){}
    NodePeer(std::shared_ptr<p2p::CapabilityHostFace> _host, const NodeID& _peerID):m_host(_host),m_id(_peerID) {}

    void sendNewStatus(u256 height, h256 genesisHash, h256 latestBlock, uint32_t version);
    void setPeerStatus(u256 height, h256 genesisHash, h256 latestBlock);

    /*
    @param id  block number or hash.
    @param flag  1, hash , 2 number
    */
    void requestBlocks(const std::vector<u256> ids, uint32_t flag);
private:
    std::shared_ptr<p2p::CapabilityHostFace> m_host;
    NodeID  m_id;

    //peer max height.
    u256 m_height;  
    //block hash.
    h256 latestBlock;
};

}  // namespace brc
}  // namespace dev