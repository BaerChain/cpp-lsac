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


void SHDposSync::addNode(p2p::NodeID id)
{
    peers.erase(id);
    auto node_peer = m_host.getNodePeer(id);

    // TODO , check block number .
    CP2P_LOG << "id : " << id << " height : " << node_peer.getHeight()
             << " latest imported: " << m_lastImportedNumber;
    if (node_peer.getHeight() < m_lastImportedNumber)
    {
        return;
    }
    std::vector<uint64_t> request_blocks;
    if (m_lastImportedNumber == 0)
    {
        request_blocks.push_back(1);
    }
    else
    {
        request_blocks.push_back(m_lastImportedNumber);
    }

    node_peer.requestBlocks(request_blocks);
}

void SHDposSync::getBlocks(p2p::NodeID id, const RLP& data)
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
                CP2P_LOG << "get data " << toHex(header);
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
                CP2P_LOG << "get data " << toHex(header);
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


void SHDposSync::BlockHeaders(p2p::NodeID id, const RLP& data)
{
    if (data.itemCount() == 0)
    {
        return;
    }
    std::vector<bytes> blocks = data[0].toVector<bytes>();
    CP2P_LOG << "import BlockHeaders " << blocks.size();
    for (size_t i = 0; i < blocks.size(); i++)
    {
        BlockHeader b(blocks[i]);
        auto import_result = m_host.bq().import(&blocks[i]);
        switch (import_result)
        {
        case ImportResult::Success: {
            CP2P_LOG << "imported block Success number:" << b.number() << " hash:" << b.hash();
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
            break;
        }
        case ImportResult::ZeroSignature: {
            break;
        }
        case ImportResult::NonceRepeat: {
            break;
        }
        default: {
            CP2P_LOG << "unkown import block result.";
        }
        }
    }
}

}  // namespace brc
}  // namespace dev