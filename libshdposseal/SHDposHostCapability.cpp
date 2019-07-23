#include "SHDposHostCapability.h"

dev::bacd::SHDposHostcapality::SHDposHostcapality(std::shared_ptr<p2p::CapabilityHostFace> _host, u256 _networkId, MsgHandler _h, OnConnHandler _oh):
    m_host(std::move(_host)),
    m_networkId(_networkId),
    m_msg_handler(_h),
    m_onconn_handler(_oh)
{

}

void dev::bacd::SHDposHostcapality::onConnect(NodeID const & _nodeID, u256 const & _peerCapabilityVersion)
{
    m_onconn_handler(_nodeID, _peerCapabilityVersion);
}

bool dev::bacd::SHDposHostcapality::interpretCapabilityPacket(NodeID const & _peerID, unsigned _id, RLP const & _r)
{
    m_msg_handler(_peerID, _id, _r);
    return true;
}
