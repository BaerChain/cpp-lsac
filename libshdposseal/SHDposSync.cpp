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
    m_lasttRequestHash = header.hash();
    m_lastRequestNumber = header.number();
    m_lastImportedNumber = m_lastRequestNumber;
}


void SHDposSync::addNode(p2p::NodeID id)
{
    peers.erase(id);

    auto node_peer = m_host.getNodePeer(id);
    std::vector<uint64_t> request_blocks;
    request_blocks.push_back(m_lastImportedNumber);
    node_peer.requestBlocks(request_blocks);
}

void SHDposSync::getBlocks(p2p::NodeID id, const RLP& data)
{
    size_t itemCount = data.itemCount();

    if (itemCount != 2)
    {
        return;
    }
    /// get block by hash
    if (data[0].toInt() == 1)
    {
    }
    /// get blocks by number
    else if (data[0].toInt() == 2)
    {
        std::vector<uint64_t> numbers = data[1].toVector<uint64_t>();
        std::vector<BlockHeader> blocks;

        CP2P_LOG << "get blocks number " << id << " data size " << numbers.size();
        if(numbers.size() > MAX_REQUEST_BLOKCS){
            return;
        }

        for (auto& itr : numbers)
        {
            auto hash = m_host.chain().numberHash(itr);
            auto header = m_host.chain().info(hash);
            blocks.push_back(header);
        }
    }
}


void SHDposSync::responseBlocks(p2p::NodeID id, const RLP& data) {

}

}  // namespace brc
}  // namespace dev