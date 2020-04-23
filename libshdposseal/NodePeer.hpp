#pragma once
#include "CommonNet.h"

namespace dev
{
namespace brcd
{

class NodePeer
{
public:
    NodePeer(std::shared_ptr<p2p::CapabilityHostFace> _host,  const NodeID& _peerID);
};

}  // namespace brcd
}  // namespace dev