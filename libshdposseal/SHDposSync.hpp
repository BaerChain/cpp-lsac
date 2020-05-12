#pragma once

#include "Common.h"
#include <libbrccore/BlockHeader.h>
#include <libp2p/Common.h>
#include <string>
namespace dev
{
namespace brc
{
class SHDposHostcapability;


struct merkleState
{
    h256 merkleHash;
    std::map<uint64_t, BlockHeader> configBlocks;
    std::map<uint64_t, bytes> cacheBlocks;
    std::vector<p2p::NodeID> nodes;

    bool haveCommon = false;
    uint64_t latestRequestNumber = 0;
    uint64_t latestImportedNumber = 0;
    uint64_t latestConfigNumber = 0;

    void updateMerkleHash()
    {
        std::vector<h256> hash;
        for (auto& itr : configBlocks)
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
    std::vector<uint64_t> request_blocks;
};


class SHDposSync
{
public:
    explicit SHDposSync(SHDposHostcapability& host);

    void addNode(const p2p::NodeID& id);

    void getBlocks(const p2p::NodeID& id, const RLP& data);
    void blockHeaders(const p2p::NodeID& id, const RLP& data);
    void newBlocks(const p2p::NodeID& id, const RLP& data);

    void updateNodeState(const p2p::NodeID& id, const RLP& data);

    SHDposSyncState getState() const { return m_state; }

    /// add broadcast block,
    void addTempBlocks(const BlockHeader& bh);

    /*
    restart sync, update all node state.
    */
    void restartSync();


private:
    //
    h256 collectBlock(const p2p::NodeID& id, const RLP& data);


    void completeSync();
    bool configNode(const p2p::NodeID& id, const RLP& data);
    void continueSync(const p2p::NodeID& id);

    SHDposHostcapability& m_host;


    std::set<p2p::NodeID> peers;
    /// config status
    std::map<h256, merkleState> m_requestStatus;
    std::map<p2p::NodeID, h256> m_nodesStatus;
    /// request status.
    std::map<p2p::NodeID, configState> m_unconfig;

    /// h256 block hash
    /// @param std::pair<bytes, uint64_t>
    ///         bytes : block data.
    ///         uint64_t: block time.  use for remove
    std::map < h256, std::pair<bytes, uint64_t> m_know_blocks_hash;

    /// h256 transaction hash
    /// @param std::pair<bytes, uint64_t>
    ///         bytes : transaction data.
    ///         uint64_t: block time.  use for remove
    std::map<h256, std::pair<bytes, uint64_t>> transactionHash;

    SHDposSyncState m_state = SHDposSyncState::None;

    uint64_t m_lastImportedNumber;
};
}  // namespace brc
}  // namespace dev