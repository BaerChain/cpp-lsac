#pragma once

#include "Common.h"
#include <libdevcore/Guards.h>
#include <libbrccore/Common.h>
#include <libp2p/Common.h>
#include <libp2p/CapabilityHost.h>
#include <libp2p/Capability.h>

namespace dev
{
class RLPStream;
namespace brc
{
    typedef std::function<void(NodeID, unsigned, RLP const&)> MsgHandler;
    typedef std::function<void(NodeID const&, u256 const&)> OnConnHandler;

    class PoaHostCapability : public p2p::CapabilityFace
    {
    public:
        PoaHostCapability(std::shared_ptr<p2p::CapabilityHostFace> _host, u256 _networkId, MsgHandler _h, OnConnHandler _oh);

        std::string name() const override { return "Poa"; }
        u256 version() const override { return c_protocolVersion; }
        unsigned messageCount() const override { return PoaPacketCount; }
        unsigned offset() const override { return PoaStatusPacket + p2p::PacketType::UserPacket; }

        void onStarting() override;
        void onStopping() override;

        void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) override;
        void onDisconnect(NodeID const& _nodeID) override;
        bool interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r) override;

        inline std::shared_ptr<p2p::CapabilityHostFace> hostFace()const { return m_host;}

        virtual void doBackgroundWork() override { };
        virtual std::chrono::milliseconds backgroundWorkInterval() const override{ return std::chrono::milliseconds{1000};};
    private:
        std::shared_ptr<p2p::CapabilityHostFace> m_host;
        u256 m_networkId;
        MsgHandler m_msg_handler;                  //消息回调
        OnConnHandler m_onconn_handler;            //连接成功回调
    };
}
}