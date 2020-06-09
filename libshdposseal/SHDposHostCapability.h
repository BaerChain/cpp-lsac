#pragma once

#include "Common.h"
#include <libbrccore/Common.h>
#include <libdevcore/Guards.h>
#include <libp2p/Capability.h>
#include <libp2p/CapabilityHost.h>
#include <libp2p/Common.h>

#include <libbrcdchain/BlockChain.h>
#include <libshdposseal/NodePeer.hpp>
#include <libshdposseal/SHDposSync.hpp>


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
        OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq, u256 _networkId,
        uint32_t version = 0);

    std::string name() const override { return "Dpos"; }
    u256 version() const override { return brc::c_protocolVersion; }
    unsigned messageCount() const override { return (unsigned)SHDposPacketType::SHDposPacketCount; }
    unsigned offset() const override { return p2p::PacketType::UserPacket + PacketCount + 1; }

    void onStarting() override {}
    void onStopping() override {}
    void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) override;
    void onDisconnect(NodeID const& /*_nodeID*/) override;
    void broadcastBlock(const h256& hash);
    void broadcastTransaction(const h256& hash);

    bool interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r) override;

    inline std::shared_ptr<p2p::CapabilityHostFace> hostFace() const { return m_host; }

    virtual void doBackgroundWork() override;
    virtual std::chrono::milliseconds backgroundWorkInterval() const override
    {
        return std::chrono::milliseconds{10};
    };

    void setBlockInterval(int64_t _time){
        if(m_sync)
            m_sync->setBlockInterval(_time);
    }

    void OnBlockImport(BlockHeader const& _info){
        broadcastBlock(_info.hash());
        m_sync->setLatestImportBlock(_info);
    }

    SHDposSyncState status() const;
    bool isSyncing() const;
    // void completeSync() { m_state = SHDposSyncState::Idle; }

    /// add
    BlockChain const& chain() const { return m_chain; }
    OverlayDB const& db() const { return m_db; }
    BlockQueue& bq() { return m_bq; }
    BlockQueue const& bq() const { return m_bq; }
    TransactionQueue const& Tq() const { return m_tq; }
    NodePeer& getNodePeer(const NodeID& id);

private:
    std::shared_ptr<p2p::CapabilityHostFace> m_host;
    u256 m_networkId;

    BlockChain const& m_chain;
    OverlayDB const& m_db;   ///< References to DB, needed for some of the BrcdChain Protocol
                             ///< responses.
    TransactionQueue& m_tq;  ///< Maintains a list of incoming transactions not yet in a block on
                             ///< the blockchain.
    BlockQueue& m_bq;  ///< Maintains a list of incoming blocks not yet on the blockchain (to be
                       ///< imported).


    /// TODO broadcastTransaction
    h256Hash m_send_txs;
    std::vector<h256> m_send_blocks;

    std::map<NodeID, NodePeer> m_peers;
    uint32_t m_version;

    std::shared_ptr<SHDposSync> m_sync;

    int64_t  m_back_fork_time = 0;    // ms
};
}  // namespace brc
}  // namespace dev
