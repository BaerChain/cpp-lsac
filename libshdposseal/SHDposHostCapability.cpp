#include "SHDposHostCapability.h"


namespace dev
{
namespace brc
{
SHDposHostcapability::SHDposHostcapability(std::shared_ptr<p2p::CapabilityHostFace> _host,
    BlockChain const& _ch, OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq,
    u256 _networkId, uint32_t version)
  : m_host(std::move(_host)),
    m_chain(_ch),
    m_db(_db),
    m_tq(_tq),
    m_bq(_bq),
    m_networkId(_networkId),
    m_version(version),
    m_sync(nullptr)
{
    m_sync.reset(new SHDposSync(*this));
}

void SHDposHostcapability::onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion)
{
    CP2P_LOG << "connect new capability";
    NodePeer np(m_host, _nodeID);
    m_peers.emplace(_nodeID, np);
    auto header = m_chain.info();
    m_peers[_nodeID].sendNewStatus(
        header.number(), m_chain.genesisHash(), header.hash(), m_version);
}


bool SHDposHostcapability::interpretCapabilityPacket(
    NodeID const& _peerID, unsigned _id, RLP const& _r)
{
    auto& peer = m_peers[_peerID];
    try
    {
        switch (_id)
        {
        case SHDposStatuspacket: {
            auto number = _r[0].toInt<u256>();
            auto genesisHash = _r[1].toHash<h256>();
            auto hash = _r[2].toHash<h256>();
            auto version = _r[3].toInt<uint32_t>();
            peer.setPeerStatus(number, genesisHash, hash, version);

            if (genesisHash == m_chain.genesisHash())
            {
                m_sync->addNode(_peerID);
            }
            else
            {
                m_host->disconnect(_peerID, p2p::UserReason);
            }
            break;
        }
        case SHDposSyncGetBlocks: {
            CP2P_LOG << "SHDposSyncGetBlocks";
            m_sync->getBlocks(_peerID, _r);
            break;
        }

        case SHDposSyncBlockHeaders: {
            peer.completeAsk();
            CP2P_LOG << "SHDposBlockHeaders";
            m_sync->blockHeaders(_peerID, _r);
            break;
        }
        case SHDposTXHash: {
            CP2P_LOG << "TODO SHDposGetTX";
            break;
        }
        case SHDposNewBlockHash: {
            if (SHDposSyncState::Idle == m_sync->getState())
            {
                CP2P_LOG << "SHDposNewBlocks";
                m_sync->newBlocks(_peerID, _r);
            }

            break;
        }
        case SHDposUpdateState:{
            
        }
        default: {
            CP2P_LOG << "cant resolve protocol.";
        }
        }
    }
    catch (const std::exception& e)
    {
        CP2P_LOG << e.what() << '\n';
    }
    return true;
}

void SHDposHostcapability::onDisconnect(NodeID const& id)
{
    m_peers.erase(id);
    m_host->disconnect(id, p2p::UserReason);
}


NodePeer &SHDposHostcapability::getNodePeer(const NodeID& id)
{
    return m_peers[id];
}


void SHDposHostcapability::broadcastBlock(const h256& hash)
{
    m_send_blocks.push_back(hash);
}

void SHDposHostcapability::broadcastTransaction(const h256& hash)
{
    m_send_txs.push_back(hash);
}

void SHDposHostcapability::doBackgroundWork()
{
    // CP2P_LOG << "TODO doBackgroundWork broadcast.";
    if (SHDposSyncState::Idle != m_sync->getState())
    {
        m_sync->restartSync();
    }
    else
    {
        if (m_send_txs.size() > 0)
        {
            for (auto& itr : m_peers)
            {
                itr.second.sendTransactionHashs(m_send_txs);
            }
        }

        // send
        if (m_send_blocks.size() > 0)
        {
            CP2P_LOG << "send blocks to ";
            for (auto& itr : m_peers)
            {
                itr.second.sendBlocksHashs(m_send_blocks);
            }
        }
    }
    m_send_txs.clear();
    m_send_blocks.clear();
}

SHDposSyncState SHDposHostcapability::status() const
{
    return m_sync->getState();
}
bool SHDposHostcapability::isSyncing() const
{
    return SHDposSyncState::Sync == m_sync->getState();
}


}  // namespace brc
}  // namespace dev
