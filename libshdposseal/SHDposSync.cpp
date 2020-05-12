#include "SHDposSync.hpp"

#include "SHDposHostCapability.h"


static const uint64_t MAX_REQUEST_BLOKCS = 128;
static const uint64_t MAX_TEMP_SYNC_BLOCKS = 1000;

namespace dev
{
namespace brc
{
SHDposSync::SHDposSync(SHDposHostcapability& host) : m_host(host), m_state(SHDposSyncState::Sync)
{
    auto header = m_host.chain().info();
    CP2P_LOG << "read from blockchain " << header.hash() << header.number();
    m_lastImportedNumber = header.number();
}


void SHDposSync::addNode(const p2p::NodeID& id)
{
    peers.insert(id);
    auto node_peer = m_host.getNodePeer(id);

    CP2P_LOG << "id : " << id << " height : " << node_peer.getHeight() << " latest imported: ";

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
        cs.request_blocks = request_blocks;
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

    std::vector<bytes> blocks = data[0].toVector<bytes>();
    std::map<uint64_t, BlockHeader> blockinfo;
    for (auto& itr : blocks)
    {
        BlockHeader b(itr);
        blockinfo[b.number()] = b;
    }

    // config common hash
    if ((blockinfo[unconfig.request_blocks[0]].hash() ==
            m_host.chain().numberHash(unconfig.request_blocks[0])) ||
        m_lastImportedNumber == 0)
    {
        if (0 == m_requestStatus.size())
        {
            merkleState ms;

            std::vector<bytes> blocks = data[0].toVector<bytes>();
            if (unconfig.request_blocks.size() != blocks.size())
            {
                CP2P_LOG << "request size (" << unconfig.request_blocks << " ) != data.size() "
                         << blocks.size();
                return true;
            }
            for (auto& itr : blocks)
            {
                BlockHeader bh(itr);
                ms.configBlocks[bh.number()] = bh;
            }

            ms.latestRequestNumber = m_lastImportedNumber;
            ms.updateMerkleHash();
            ms.latestImportedNumber = m_lastImportedNumber;
            // caculate merkle hash.
            m_unconfig.erase(id);

            m_nodesStatus[id] = ms.merkleHash;
            m_requestStatus[ms.merkleHash] = ms;

            continueSync(id);
        }
    }
    else
    {
        // find common hash.
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

    /// get block by number
    if (1 == data[0].toInt())
    {
        std::vector<uint64_t> numbers = data[1].toVector<uint64_t>();

        CP2P_LOG << "get blocks number " << id << " data size " << numbers.size();
        uint64_t max_number = m_host.chain().info().number();
        for (uint64_t i = 0; i < numbers.size() && i < MAX_REQUEST_BLOKCS; i++)
        {
            try
            {
                if (numbers[i] <= max_number)
                {
                    auto hash = m_host.chain().numberHash(numbers[i]);
                    auto header = m_host.chain().block(hash);
                    blocks.push_back(header);
                }
            }
            catch (...)
            {
                CP2P_LOG << "get block exception  by number.";
            }
        }
    }
    /// get blocks by hash
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

h256 SHDposSync::collectBlock(const p2p::NodeID& id, const RLP& data)
{
    auto& hash = m_nodesStatus[id];
    auto& requestState = m_requestStatus[hash];

    std::vector<bytes> blocks = data[0].toVector<bytes>();
    std::vector<BlockHeader> hb;


    for (size_t i = 0; i < blocks.size(); i++)
    {
        BlockHeader b(blocks[i]);
        requestState.cacheBlocks[b.number()] = blocks[i];
    }
    //

    requestState.latestConfigNumber = requestState.cacheBlocks.rbegin()->first;

    return hash;
}
void SHDposSync::blockHeaders(const p2p::NodeID& id, const RLP& data)
{
    /// config node state
    if (m_unconfig.count(id))
    {
        configNode(id, data);
        return;
    }

    auto merkleHash = collectBlock(id, data);
    auto& requestState = m_requestStatus[merkleHash];
    if (requestState.latestConfigNumber > requestState.latestImportedNumber)
    {
        CP2P_LOG << "import blocks " << requestState.cacheBlocks.size();
        for (uint64_t start = requestState.latestImportedNumber + 1;
             start <= requestState.latestConfigNumber; start++)
        {
            RLPStream rs;

            auto bh = requestState.cacheBlocks[start];
            auto import_result = m_host.bq().import(&bh);
            switch (import_result)
            {
            case ImportResult::Success: {
                requestState.latestImportedNumber = start;
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
                // CP2P_LOG << "AlreadyKnown " << b.number() << " hash: " << b.hash();
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
       
        auto iter = requestState.cacheBlocks.begin();
        while (iter != requestState.cacheBlocks.end())
        {
            if (iter->first < requestState.latestConfigNumber)
            {
                iter = requestState.cacheBlocks.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
    continueSync(id);
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
    if (m_host.getNodePeer(id).getAksing() == SHposAsking::Request)
    {
        return;
    }
    auto& hash = m_nodesStatus[id];
    auto& requestState = m_requestStatus[hash];

    auto bheader = requestState.cacheBlocks.rbegin();
    if (bheader != requestState.cacheBlocks.rend())
    {
        BlockHeader bh(bheader->second);
        if (utcTimeMilliSec() - bh.timestamp() < 20 * 1000)
        {
            m_state = SHDposSyncState::Idle;
            return;
        }
    }


    // config blockQueue m_readySet.size() < MAX_TEMP_SYNC_BLOCKS
    if (m_host.bq().items().first >= MAX_TEMP_SYNC_BLOCKS)
    {
        CP2P_LOG << "wait import ...";
        m_state = SHDposSyncState::Waiting;
        return;
    }

    auto start_requrest = requestState.latestRequestNumber;
    start_requrest = start_requrest == 0 ? 1 : start_requrest;
    auto end_request = start_requrest + MAX_REQUEST_BLOKCS;

    std::vector<uint64_t> numbers;

    for (auto i = start_requrest; i < end_request; i++)
    {
        numbers.push_back(i);
    }
    requestState.latestRequestNumber = end_request;
    m_host.getNodePeer(id).requestBlocks(numbers);
}

void SHDposSync::restartSync()
{
    if (m_state == SHDposSyncState::Waiting)
    {
        if (m_host.bq().items().first <= MAX_TEMP_SYNC_BLOCKS)
        {
            for (auto& itr : peers)
            {
                // if(itr.getAksing() == SHposAsking::Nothing){

                // }
                continueSync(itr);
            }
        }
    }
    else if (m_state == SHDposSyncState::Sync)
    {
        if (peers.size() == 0)
        {
            completeSync();
        }
    }
    // else
    // {
    //     CP2P_LOG << "ignore restart sync";
    // }
}

void SHDposSync::updateNodeState(const p2p::NodeID& id, const RLP& data)
{
    auto& peer = m_host.getNodePeer(id);
    auto number = data[0].toInt<u256>();
    auto genesisHash = data[1].toHash<h256>();
    auto hash = data[2].toHash<h256>();
    auto version = data[3].toInt<uint32_t>();
    peer.setPeerStatus(number, genesisHash, hash, version);

    // TODO update peer .
}

void SHDposSync::addTempBlocks(const BlockHeader& bh) {}


void SHDposSync::completeSync()
{
    CP2P_LOG << "complete sync";
    m_state = SHDposSyncState::Idle;
}


}  // namespace brc
}  // namespace dev