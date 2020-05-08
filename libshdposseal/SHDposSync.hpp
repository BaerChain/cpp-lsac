#pragma once

#include <libbrccore/BlockHeader.h>
#include <libp2p/Common.h>
#include <string>
namespace dev
{
namespace brc
{
class SHDposHostcapability;

enum class syncState
{
    sync = 0x1,
    wait
};


struct merkleState
{
    h256 merkleHash;
    std::map<uint64_t, BlockHeader> blocks;
    std::vector<p2p::NodeID> nodes;

    uint64_t m_latestRequest = 0;


    bool isNull() const
    {
        return blocks.size() == 0 && merkleHash == h256() && nodes.size() == 0 &&
               m_latestRequest == 0;
    }

    void updateMerkleHash()
    {
        std::vector<h256> hash;
        for (auto& itr : blocks)
        {
            hash.push_back(itr.second.hash());
        }

        while (hash.size() != 1)
        {
            std::vector<h256> temp;
            for (size_t i = 0; i < hash.size() / 2; i++)
            {
                auto i1 = hash[i * 2];
                auto i2 = hash[i * 2 + 1];
                bytes d;
                d.resize(64);

                memcpy(d.data(), i1.data(), 32);
                memcpy(d.data() + 32, i2.data(), 32);

                auto ret = sha3(d);
                temp.push_back(ret);
            }
            hash.clear();
            hash = temp;
        }
    }
};

struct configState
{
    p2p::NodeID id;
    std::map<h256, std::vector<uint64_t>> request_blocks;
};


class SHDposSync
{
public:
    explicit SHDposSync(SHDposHostcapability& host);

    void addNode(const p2p::NodeID& id);

    void getBlocks(const p2p::NodeID& id, const RLP& data);
    void blockHeaders(const p2p::NodeID& id, const RLP& data);
    void newBlocks(const p2p::NodeID& id, const RLP& data);

    void restartSync();

private:
    bool configNode(const p2p::NodeID& id, const RLP& data);
    void continueSync(const p2p::NodeID& id);

    SHDposHostcapability& m_host;


    uint32_t m_lastRequestNumber = 0;
    h256 m_lasttRequestHash;
    uint32_t m_lastImportedNumber = 0;
    uint64_t m_latestImportedBlockTime = 0;

    ///
    std::map<p2p::NodeID, std::map<uint64_t, u256>> m_sendBlocks;

    // Block numbers , block hash
    std::map<uint64_t, h256> m_recevieBlocks;

    std::set<p2p::NodeID> peers;

    /// config status
    std::map<h256, merkleState> m_requestStatus;
    std::map<p2p::NodeID, h256> m_nodesStatus;

    std::map<h256, uint64_t> m_know_blocks_hash;
    std::map<uint64_t, std::set<h256>> m_blocks_hash;
    std::set<h256> transactionHash;


    /// request status.
    std::map<p2p::NodeID, configState> m_unconfig;
    syncState m_state;
};
}  // namespace brc
}  // namespace dev