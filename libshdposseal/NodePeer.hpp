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

    void sendNewStatus(u256 height, h256 genesisHash, h256 latestBlock);
private:
    std::shared_ptr<p2p::CapabilityHostFace> m_host;
    NodeID  m_id;
};

}  // namespace brc
}  // namespace dev