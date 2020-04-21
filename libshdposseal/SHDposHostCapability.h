#pragma once

#include "Common.h"
#include <libbrccore/Common.h>
#include <libdevcore/Guards.h>
#include <libp2p/Capability.h>
#include <libp2p/CapabilityHost.h>
#include <libp2p/Common.h>


namespace dev
{
class RLPStream;
namespace brc
{
class TransactionQueue;
class BlockQueue;
class BlockChainSync;


typedef std::function<void(NodeID, unsigned, RLP const&)> MsgHandler;
typedef std::function<void(NodeID const&, u256 const&)> OnConnHandler;

class SHDposHostcapability : public p2p::CapabilityFace
{
public:
    SHDposHostcapability(std::shared_ptr<p2p::CapabilityHostFace> _host, BlockChain const& _ch,
        OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq, u256 _networkId);

    std::string name() const override { return "Dpos"; }
    u256 version() const override { return brc::c_protocolVersion; }
    unsigned messageCount() const override { return SHDposPacketCount; }
    unsigned offset() const override { return SHDposStatuspacket + p2p::PacketType::UserPacket; }

    void onStarting() override {}
    void onStopping() override {}
    void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) override;
    void onDisconnect(NodeID const& /*_nodeID*/) override {}

    bool interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r) override;

    inline std::shared_ptr<p2p::CapabilityHostFace> hostFace() const { return m_host; }

    virtual void doBackgroundWork() override{};
    virtual std::chrono::milliseconds backgroundWorkInterval() const override
    {
        return std::chrono::milliseconds{500};
    };


private:
    std::shared_ptr<p2p::CapabilityHostFace> m_host;
    u256 m_networkId;
    
    BlockChain const& m_chain;
    OverlayDB const& m_db;                    ///< References to DB, needed for some of the BrcdChain Protocol responses.
    TransactionQueue& m_tq;                    ///< Maintains a list of incoming transactions not yet in a block on the blockchain.
    BlockQueue& m_bq;                        ///< Maintains a list of incoming blocks not yet on the blockchain (to be imported).
};
}  // namespace brc
}  // namespace dev
