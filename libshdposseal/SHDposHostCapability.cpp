#include "SHDposHostCapability.h"
#include "libbrcdchain/TransactionQueue.h"


namespace dev
{
namespace brc
{
SHDposHostcapability::SHDposHostcapability(std::shared_ptr<p2p::CapabilityHostFace> _host,
    BlockChain const& _ch, OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq, 
    u256 _networkId, ex::exchange_plugin& _exdb, uint32_t version)
  : m_host(std::move(_host)),
    m_chain(_ch),
    m_db(_db),
    m_exdb(_exdb),
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
                cwarn << "Invalid genesisHash disconnect:"<< _peerID;
                m_host->disconnect(_peerID, p2p::UserReason);
            }
            break;
        }
        case SHDpodUpNodeConfig:{
            CP2P_LOG << "SHDpodUpNodeConfig";
            m_sync->getConfigBlocks(_peerID, _r);
            break;
        }
        case SHDpodNodeConfig:{
            CP2P_LOG << "SHDpodNodeConfig";
            m_sync->configNode(_peerID, _r);
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
        case SHDposBroadcastTXHash: {
            CP2P_LOG << "get transaction by hash.";
            CP2P_LOG << toJS(_r.data()) << _r.itemCount();
            m_sync->getTransaction(_peerID, _r);
            break;
        }
        case SHDposGetTransactionsForRequest: {
            CP2P_LOG << "get transaction for request.";
            m_sync->sendTransaction(_peerID, _r);
            break;
        }
        case SHDposGetTransactions: {
            CP2P_LOG << "get transaction.";
            m_sync->importedTransaction(_peerID, _r, chain(), m_db, m_exdb);
            break;
        }
        case SHDposNewBlockHash: {
            if (SHDposSyncState::Idle == m_sync->getState())
            {
                CP2P_LOG << "SHDposNewBlocks";
                m_sync->newBlocks(_peerID, _r);
            } else{
                CP2P_LOG << "get new block hash and to resuest state";
                getNodePeer(_peerID).requestState();
            }
            break;
        }
        case SHDposGetState: {
            auto header = m_chain.info();
            m_peers[_peerID].sendState(
                header.number(), m_chain.genesisHash(), header.hash(), m_version);
            break;
        }
        case SHDposUpdateState: {
            auto number = _r[0].toInt<u256>();
            auto genesisHash = _r[1].toHash<h256>();
            auto hash = _r[2].toHash<h256>();
            auto version = _r[3].toInt<uint32_t>();
            CP2P_LOG << "update state, continue sync. number:"<< number;
            peer.completeAsk();
            peer.setPeerStatus(number, genesisHash, hash, version);
            if (SHDposSyncState::Idle != m_sync->getState())
            {
                m_sync->restartSync();
            }
            break;
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
    CP2P_LOG << "disconnect " << id ;
    m_sync->removeNode(id);
    m_peers.erase(id);
    m_host->disconnect(id, p2p::UserReason);
}


NodePeer& SHDposHostcapability::getNodePeer(const NodeID& id)
{
    return m_peers[id];
}


void SHDposHostcapability::broadcastBlock(const h256& hash)
{
    m_send_blocks.push_back(hash);
}

void SHDposHostcapability::broadcastTransaction(const h256& hash)
{
    m_send_txs.insert(hash);
}

void SHDposHostcapability::doBackgroundWork()
{
    //CP2P_LOG << "TODO doBackgroundWork broadcast. state:"<< (int)m_sync->getState();
    if (SHDposSyncState::Idle != m_sync->getState())
    {
        m_sync->restartSync();
    }
    else
    {
        if (m_newTransactions)
        {
            h256Hash _avoid;
            std::vector<h256> stx;
            auto transactions = m_tq.topTransactions(0, _avoid);
            for (auto& itr : transactions)
            {
                stx.push_back(itr.sha3());
            }

            for (auto& itr : m_peers)
            {
                CP2P_LOG << "send tarnsaction to :"<<itr.first;
                itr.second.sendTransactionHashs(stx);
            }
            m_sync->addKnowTransaction(stx);
            m_newTransactions = false;
        }

        // send
        if (m_send_blocks.size() > 0)
        {
            //CP2P_LOG << "send blocks to ";
            for (auto& itr : m_peers)
            {
                itr.second.sendBlocksHashs(m_send_blocks);
            }
            m_sync->addKnowBlock(m_send_blocks);

        }
    }

//    m_back_fork_time += backgroundWorkInterval().count();
//    m_back_fork_time = 0;
    /// dell the fork block and set the looptimes
    for(auto const& p: m_peers)
        m_sync->backForkBlock(p.first);
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

