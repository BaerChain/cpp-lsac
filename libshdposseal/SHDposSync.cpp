#include "SHDposSync.hpp"

#include "SHDposHostCapability.h"


static const uint64_t MAX_REQUEST_BLOKCS = 128;


namespace dev
{
namespace brc
{
SHDposSync::SHDposSync(SHDposHostcapability& host) : m_host(host)
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
    }
}


void SHDposSync::addNode(const p2p::NodeID& id)
{
    peers.erase(id);
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
        node_peer.requestBlocksHash(request_blocks);

        configState cs{id, {request_blocks}};
        m_unconfig[id] = cs;
    }
    else
    {
    }
}


void SHDposSync::configNode(const p2p::NodeID& id, const RLP& data)
{
    if (!m_unconfig.count(id))
    {
        CP2P_LOG << "cant find id from config.";
        return;
    }


    
}


void SHDposSync::getBlocks(const p2p::NodeID& id, const RLP& data)
{
    size_t itemCount = data.itemCount();

    if (itemCount != 2)
    {
        return;
    }

    CP2P_LOG << "getBlocks " << id << " item " << itemCount << " type: " << data[0].toInt();

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

        for (uint64_t i = 0; i < hashs.size() && i < MAX_REQUEST_BLOKCS; i++)
        {
            try
            {
                auto header = m_host.chain().block(hashs[i]);
                blocks.push_back(header);
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
    if (0 == data.itemCount())
    {
        return;
    }
    std::vector<bytes> blocks = data[0].toVector<bytes>();
    for (size_t i = 0; i < blocks.size(); i++)
    {
        BlockHeader b(blocks[i]);
        auto import_result = m_host.bq().import(&blocks[i]);
        switch (import_result)
        {
        case ImportResult::Success: {
            m_lastImportedNumber = b.number();
            if (0 == m_lastImportedNumber % 1000)
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
    // TODO continue sync block.
    continueSync(id);
}


void SHDposSync::newBlocks(const p2p::NodeID& id, const RLP& data) {}

void SHDposSync::continueSync(const p2p::NodeID& id)
{
    ///
    CP2P_LOG << "continue sync from " << m_lastRequestNumber;
    std::vector<uint64_t> numbers;
    for (uint64_t i = m_lastRequestNumber; i < m_lastRequestNumber + MAX_REQUEST_BLOKCS; i++)
    {
        numbers.push_back(i);
    }
    m_lastRequestNumber += MAX_REQUEST_BLOKCS;
    m_host.getNodePeer(id).requestBlocks(numbers);
}


}  // namespace brc
}  // namespace dev