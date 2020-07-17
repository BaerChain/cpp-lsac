#pragma once

#include "CommonNet.h"
#include "BrcdChainPeer.h"
#include <libdevcore/Guards.h>
#include <libdevcore/OverlayDB.h>
#include <libbrccore/BlockHeader.h>
#include <libbrccore/Common.h>
#include <libbrcdchain/BlockChainSync.h>
#include <libp2p/Capability.h>
#include <libp2p/CapabilityHost.h>
#include <libp2p/Common.h>
#include <indexDb/database/include/brc/exchangeOrder.hpp>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dev
{

class RLPStream;

namespace brc
{

class TransactionQueue;
class BlockQueue;
class BlockChainSync;

class BrcdChainPeerObserverFace
{
public:
    virtual ~BrcdChainPeerObserverFace() = default;

    virtual void onPeerStatus(BrcdChainPeer const& _peer) = 0;

    virtual void onPeerTransactions(NodeID const& _peerID, RLP const& _r, BlockChain const& _blockChain, OverlayDB const& _db, ex::exchange_plugin const& _exdb) = 0;

    virtual void onPeerBlockHeaders(NodeID const& _peerID, RLP const& _headers) = 0;

    virtual void onPeerBlockBodies(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerNewHashes(
        NodeID const& _peerID, std::vector<std::pair<h256, u256>> const& _hashes) = 0;

    virtual void onPeerNewBlock(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerNodeData(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerReceipts(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerAborting() = 0;
};

class BrcdChainHostDataFace
{
public:
    virtual ~BrcdChainHostDataFace() = default;

    virtual std::pair<bytes, unsigned> blockHeaders(
        RLP const& _blockId, unsigned _maxHeaders, u256 _skip, bool _reverse) const = 0;

    virtual std::pair<bytes, unsigned> blockBodies(RLP const& _blockHashes) const = 0;

    virtual strings nodeData(RLP const& _dataHashes) const = 0;

    virtual std::pair<bytes, unsigned> receipts(RLP const& _blockHashes) const = 0;
};


/**
 * @brief The BrcdChainCapability class
 * @warning None of this is thread-safe. You have been warned.
 * @doWork Syncs to peers and sends new blocks and transactions.
 */
class BrcdChainCapability : public p2p::CapabilityFace
{
public:
    /// Start server, but don't listen.
    BrcdChainCapability(std::shared_ptr<p2p::CapabilityHostFace> _host, BlockChain const& _ch,
        OverlayDB const& _db, ex::exchange_plugin const& _exdb,TransactionQueue& _tq, BlockQueue& _bq, u256 _networkId);

    std::string name() const override { return "brc"; }
    u256 version() const override { return c_protocolVersion; }
    unsigned messageCount() const override { return PacketCount; }
    unsigned offset() const override { return p2p::PacketType::UserPacket; }

    void onStarting() override;
    void onStopping() override;

    unsigned protocolVersion() const { return c_protocolVersion; }
    u256 networkId() const { return m_networkId; }
    void setNetworkId(u256 _n) { m_networkId = _n; }

    void reset();
    /// Don't sync further - used only in test mode
    void completeSync();

    bool isSyncing() const;

    void noteNewTransactions() { m_newTransactions = true; }
	void noteNewBlocks() { m_newBlocks = true; }
	void noteNewBlocksSend() { m_newSend = true; }
    void onBlockImported(BlockHeader const& _info) { m_sync->onBlockImported(_info); }

    BlockChain const& chain() const { return m_chain; }
    OverlayDB const& db() const { return m_db; }
    BlockQueue& bq() { return m_bq; }
    BlockQueue const& bq() const { return m_bq; }
    SyncStatus status() const;

    static char const* stateName(SyncState _s) { return s_stateNames[static_cast<int>(_s)]; }

    static unsigned const c_oldProtocolVersion;

    void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) override;
    void onDisconnect(NodeID const& _nodeID) override;
    bool interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r) override;

    p2p::CapabilityHostFace& capabilityHost() { return *m_host; }

    BrcdChainPeer const& peer(NodeID const& _peerID) const;
    BrcdChainPeer& peer(NodeID const& _peerID);
    void disablePeer(NodeID const& _peerID, std::string const& _problem);

    virtual void doBackgroundWork() override;

    std::chrono::milliseconds backgroundWorkInterval()  const override { return std::chrono::milliseconds{10};}

    void debugMemery();
private:
    static char const* const s_stateNames[static_cast<int>(SyncState::Size)];

    std::tuple<std::vector<NodeID>, std::vector<NodeID>> randomSelection(unsigned _percent = 25,
        std::function<bool(BrcdChainPeer const&)> const& _allow = [](BrcdChainPeer const&) {
            return true;
        });



    void maintainTransactions();
    void maintainBlocks(h256 const& _currentBlock);
    void onTransactionImported(ImportResult _ir, h256 const& _h, h512 const& _nodeId);

    /// Initialises the network peer-state, doing the stuff that needs to be once-only. @returns true if it really was first.
    bool ensureInitialised();

    void setIdle(NodeID const& _peerID);
    void setAsking(NodeID const& _peerID, Asking _a);

    /// Are we presently in a critical part of the syncing process with this peer?
    bool isCriticalSyncing(NodeID const& _peerID) const;

    /// Do we presently need syncing with this peer?
    bool needsSyncing(NodeID const& _peerID) const;

	void sendNewBlock();
    std::shared_ptr<p2p::CapabilityHostFace> m_host;

    BlockChain const& m_chain;
    OverlayDB const& m_db;                    ///< References to DB, needed for some of the BrcdChain Protocol responses.
    ex::exchange_plugin const& m_exdb;
    TransactionQueue& m_tq;                    ///< Maintains a list of incoming transactions not yet in a block on the blockchain.
    BlockQueue& m_bq;                        ///< Maintains a list of incoming blocks not yet on the blockchain (to be imported).

    u256 m_networkId;

    h256 m_latestBlockSent;
    h256Hash m_transactionsSent;

    std::atomic<bool> m_newTransactions = {false};
    std::atomic<bool> m_newBlocks = {false};

	std::atomic<bool> m_newSend = { false };

    std::shared_ptr<BlockChainSync> m_sync;
    std::atomic<time_t> m_lastTick = { 0 };

    std::unique_ptr<BrcdChainHostDataFace> m_hostData;
    std::unique_ptr<BrcdChainPeerObserverFace> m_peerObserver;

    std::unordered_map<NodeID, BrcdChainPeer> m_peers;

    std::atomic<bool> m_backgroundWorkEnabled = {false};

    std::mt19937_64 m_urng;  // Mersenne Twister psuedo-random number generator

    Logger m_logger{createLogger(VerbosityDebug, "brccap")};
    /// Logger for messages about impolite behaivour of peers.
    Logger m_loggerImpolite{createLogger(VerbosityDebug, "impolite")};
	Logger m_log_test{createLogger(VerbosityInfo, "test_info")};
};

}
}
