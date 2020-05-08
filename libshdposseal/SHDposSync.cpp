#include "SHDposSync.hpp"

#include "SHDposHostCapability.h"


static const uint64_t MAX_REQUEST_BLOKCS = 128;
static const uint64_t MAX_TEMP_SYNC_BLOCKS = 1000;

namespace dev
{
namespace brc
{
SHDposSync::SHDposSync(SHDposHostcapability& host) : m_host(host), m_state(syncState::sync)
{
    auto header = m_host.chain().info();
    CP2P_LOG << "read from blockchain " << header.hash() << header.number();
    if (header.number() == 0)
    {
        m_lastRequestNumber = 0;
        m_lastImportedNumber = 0;
    }
    else
    {
        m_lasttRequestHash = header.hash();
        m_lastRequestNumber = header.number();
        m_lastImportedNumber = m_lastRequestNumber;
        m_latestImportedBlockTime = header.timestamp();
    }
}


void SHDposSync::addNode(const p2p::NodeID& id)
{
    peers.insert(id);
    auto node_peer = m_host.getNodePeer(id);


    CP2P_LOG << "id : " << id << " height : " << node_peer.getHeight()
             << " latest imported: " << m_lastImportedNumber;

    auto height = node_peer.getHeight();


    // config same chain.
    if (0 == m_requestStatus.size())
    {
        std::vector<uint64_t> request_blocks;
        if (12 >= height)
        {
            request_blocks.emplace_back(1);
        }
        else
        {
            height -= 12;

            /// m_lastImportedNumber, 1/4, 1/2, 3/4, height -12.
            request_blocks.emplace_back(m_lastImportedNumber > 0 ? m_lastImportedNumber : 1);
            request_blocks.emplace_back(height / 4);
            request_blocks.emplace_back(height / 2);
            request_blocks.emplace_back(height * 3 / 4);
            request_blocks.emplace_back(height);
        }
        configState cs;
        cs.id = id;
        cs.request_blocks[h256()] = request_blocks;
        m_unconfig[id] = cs;

        node_peer.requestBlocks(request_blocks);
    }
    else
    {
        CP2P_LOG << "TODO.";
    }
}


bool SHDposSync::configNode(const p2p::NodeID& id, const RLP& data)
{
    CP2P_LOG << "config node ";
    auto unconfig = m_unconfig[id];

    if (0 == m_requestStatus.size())
    {
        if (!unconfig.request_blocks.count(h256()))
        {
            CP2P_LOG << "bad status.";
            return true;
        }
        merkleState ms;

        std::vector<bytes> blocks = data[0].toVector<bytes>();
        if (unconfig.request_blocks[h256()].size() != blocks.size())
        {
            CP2P_LOG << "request size (" << unconfig.request_blocks << " ) != data.size() "
                     << blocks.size();
            return true;
        }
        std::vector<h256> blocksHash;

        std::map<uint64_t, BlockHeader> blockinfo;
        for (auto& itr : blocks)
        {
            BlockHeader b(itr);
            blockinfo[b.number()] = b;
        }

        auto td_blocks = unconfig.request_blocks[h256()];
        // get hash from data.
        for (auto& itr : td_blocks)
        {
            if (!blockinfo.count(itr))
            {
                CP2P_LOG << "cant find request number.";
                return true;
            }
        }

        ms.blocks = blockinfo;
        ms.updateMerkleHash();
        ms.m_latestRequest = m_lastImportedNumber;
        // caculate merkle hash.
        m_unconfig.erase(id);

        m_nodesStatus[id] = ms.merkleHash;
        m_requestStatus[ms.merkleHash] = ms;
        continueSync(id);
    }
    else
    {
        for (auto& itr : unconfig.request_blocks)
        {
        }
    }
    return true;
}


void SHDposSync::getBlocks(const p2p::NodeID& id, const RLP& data)
{
    size_t itemCount = data.itemCount();

    if (itemCount != 2)
    {
        return;
    }


    std::vector<bytes> blocks;

    /// get block by hash
    if (1 == data[0].toInt())
    {
        std::vector<uint64_t> numbers = data[1].toVector<uint64_t>();

        CP2P_LOG << "get blocks number " << id << " data size " << numbers.size();

        for (uint64_t i = 0; i < numbers.size() && i < MAX_REQUEST_BLOKCS; i++)
        {
            try
            {
                auto hash = m_host.chain().numberHash(numbers[i]);
                auto header = m_host.chain().block(hash);
                blocks.push_back(header);
            }
            catch (...)
            {
                CP2P_LOG << "get block exception  by number.";
            }
        }
    }
    /// get blocks by number
    else if (2 == data[0].toInt())
    {
        std::vector<h256> hashs = data[1].toVector<h256>();
        CP2P_LOG << "get blocks hash " << id << " data size " << hashs;
        for (uint64_t i = 0; i < hashs.size() && i < MAX_REQUEST_BLOKCS; i++)
        {
            try
            {
                if (hashs[i] != h256())
                {
                    CP2P_LOG << "find " << hashs[i];
                    auto header = m_host.chain().block(hashs[i]);
                    blocks.push_back(header);
                }
            }
            catch (...)
            {
                CP2P_LOG << "get block exception by hash.";
            }
        }
    }
    m_host.getNodePeer(id).sendBlocks(blocks);
}


void SHDposSync::blockHeaders(const p2p::NodeID& id, const RLP& data)
{
    /// config node state
    if (m_unconfig.count(id))
    {
        configNode(id, data);
        return;
    }

    std::vector<bytes> blocks = data[0].toVector<bytes>();
    for (size_t i = 0; i < blocks.size(); i++)
    {
        BlockHeader b(blocks[i]);
        auto import_result = m_host.bq().import(&blocks[i]);

        m_know_blocks_hash[b.hash()] = b.number();

        if (i == 0 || (blocks.size() - 1 == i))
        {
            CP2P_LOG << "start or end import " << i << " n:" << b.number();
        }

        switch (import_result)
        {
        case ImportResult::Success: {
            m_lastImportedNumber = b.number();
            if (0 == (m_lastImportedNumber % 1000))
            {
                CP2P_LOG << "imported block Success number:" << b.number() << " hash:" << b.hash();
            }

            break;
        }
        case ImportResult::UnknownParent: {
            CP2P_LOG << "UnknownParent";
            break;
        }
        case ImportResult::FutureTimeKnown: {
            CP2P_LOG << "FutureTimeKnown";
            break;
        }
        case ImportResult::FutureTimeUnknown: {
            CP2P_LOG << "FutureTimeUnknown";
            break;
        }
        case ImportResult::AlreadyInChain: {
            CP2P_LOG << "AlreadyInChain";
            break;
        }
        case ImportResult::AlreadyKnown: {
            CP2P_LOG << "AlreadyKnown";
            break;
        }
        case ImportResult::Malformed: {
            CP2P_LOG << "Malformed";
            break;
        }
        case ImportResult::OverbidGasPrice: {
            CP2P_LOG << "OverbidGasPrice";
            break;
        }
        case ImportResult::BadChain: {
        }
        case ImportResult::ZeroSignature: {
        }
        case ImportResult::NonceRepeat: {
        }
        default: {
            CP2P_LOG << "unkown import block result.";
        }
        }
    }

    for (auto& itr : m_know_blocks_hash)
    {
        if (itr.second < m_lastImportedNumber - 60)
        {
            m_know_blocks_hash.erase(itr.first);
        }
    }


    if (m_host.status() == SHDposSyncState::Sync)
    {
        BlockHeader b(blocks.back());
        m_latestImportedBlockTime = b.timestamp();

        CP2P_LOG << "complete sync." << utcTimeMilliSec() - b.timestamp();
        if (utcTimeMilliSec() - b.timestamp() < 21000)
        {
            ///
            CP2P_LOG << "complete sync." << utcTimeMilliSec() - b.timestamp();
            m_host.completeSync();
        }
        else
        {
            continueSync(id);
        }
    }
}


void SHDposSync::newBlocks(const p2p::NodeID& id, const RLP& data)
{
    std::vector<h256> _hash = data[0].toVector<h256>();
    CP2P_LOG << "newBlocks  " << _hash;
    std::vector<h256> unkownHash;

    for (auto& itr : _hash)
    {
        if (!m_know_blocks_hash.count(itr) && itr != h256())
        {
            unkownHash.push_back(itr);
        }
    }

    if (unkownHash.size() > 0)
    {
        m_host.getNodePeer(id).requestBlocks(unkownHash);
    }
}

void SHDposSync::continueSync(const p2p::NodeID& id)
{
    auto& hash = m_nodesStatus[id];
    auto& requestState = m_requestStatus[hash];

    // config blockQueue m_readySet.size() < MAX_TEMP_SYNC_BLOCKS
    if (m_host.bq().items().first >= MAX_TEMP_SYNC_BLOCKS)
    {
        CP2P_LOG << "wait import ...";
        m_state = syncState::wait;
        return;
    }

    int64_t x_number = (utcTimeMilliSec() - m_latestImportedBlockTime) / 1000;

    uint64_t request_number = MAX_REQUEST_BLOKCS;
    if (x_number > 0)
    {
        request_number = std::min<uint64_t>((uint64_t)x_number + 1, MAX_REQUEST_BLOKCS);
    }
    // proccess 0 .
    requestState.m_latestRequest =
        requestState.m_latestRequest == 0 ? 1 : requestState.m_latestRequest;

    CP2P_LOG << "continue sync from " << requestState.m_latestRequest;
    std::vector<uint64_t> numbers;

    for (uint64_t i = requestState.m_latestRequest;
         i < requestState.m_latestRequest + request_number; i++)
    {
        numbers.push_back(i);
    }
    requestState.m_latestRequest += request_number;
    m_host.getNodePeer(id).requestBlocks(numbers);
}

void SHDposSync::restartSync()
{
    if (m_state == syncState::wait)
    {
        if (m_host.bq().items().first <= MAX_TEMP_SYNC_BLOCKS)
        {
            CP2P_LOG << "m_host.bq().items().first " << m_host.bq().items().first << " peers "
                     << peers.size();
            for (auto& itr : peers)
            {
                continueSync(itr);
            }
        }
    }
    // else
    // {
    //     CP2P_LOG << "ignore restart sync";
    // }
}


}  // namespace brc
}  // namespace dev