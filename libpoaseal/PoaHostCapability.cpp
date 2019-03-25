#include "PoaHostCapability.h"

#include <libdevcore/Common.h>
#include <libethcore/Exceptions.h>
#include <libp2p/Host.h>
#include <libp2p/Session.h>
#include <chrono>
#include <thread>

dev::eth::PoaHostCapability::PoaHostCapability(std::shared_ptr<p2p::CapabilityHostFace> _host,u256 _networkId, MsgHandler _h, OnConnHandler _oh)
	: m_host(std::move(_host)),
	m_networkId(_networkId),
    m_msg_handler(_h),
    m_onconn_handler(_oh)
{

}

void dev::eth::PoaHostCapability::onStarting() 
{
	cdebug << "this in onString() func ...";
}

void dev::eth::PoaHostCapability::onStopping()
{
	cdebug << "this in onStopping() func ...";
}

void dev::eth::PoaHostCapability::onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion)
{	
    //TODO 连接成功后 同步验证人列表 可以发送状态消息 同步验证人列表
	cdebug << "connection is ok...";
	m_onconn_handler(_nodeID, _peerCapabilityVersion);
}

void dev::eth::PoaHostCapability::onDisconnect(NodeID const& /*_nodeID*/) {}

bool dev::eth::PoaHostCapability::interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r)
{
	m_msg_handler(_peerID, _id, _r);
    return true;
}
