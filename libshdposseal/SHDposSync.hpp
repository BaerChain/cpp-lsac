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
    std::set<p2p::NodeID> nodes;

    uint64_t latestRequestNumber = 0;
    uint64_t latestConfigNumber = 0;
    uint64_t latestImportNumber = 0;
    BlockHeader latestImportBlock;

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
    uint64_t  height = 0;
};




class SHDposSync
{
public:
    explicit SHDposSync(SHDposHostcapability& host);

    void addNode(const p2p::NodeID& id);

    /// other node get block .
    void getBlocks(const p2p::NodeID& id, const RLP& data);
    /// other node get transaction.
    void getTransaction(const p2p::NodeID& id, const RLP& data);

    /// get transaction from node.
    void importedTransaction(const p2p::NodeID& id, const RLP& data);

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

    void addKnowBlock(const std::vector<h256>& hash);
    void addKnowTransaction(const std::vector<h256>& hash);

    void removeNode(const p2p::NodeID& id);

    void setBlockInterval(int64_t _time) { m_block_interval = _time; }

    void setLatestImportBlock(BlockHeader const& _h) { m_latestImportBlock = _h;}

private:
    /// sync block
    void syncBlock(const p2p::NodeID& id, const RLP& data);

    ///
    void processBlock(const p2p::NodeID& id, const RLP& data);

    //
    h256 collectBlockSync(const p2p::NodeID& id, const RLP& data);

    bool importedBlock(const bytes& data);

    ///@param expire  expire time.
    void clearTemp(uint64_t expire = 60 * 1000);


    void completeSync();
    bool configNode(const p2p::NodeID& id, const RLP& data);
    void continueSync(const p2p::NodeID& id);

    bool isKnowInChain(h256 const& _hash) const;

    int64_t  getBlockInterval() const { return m_block_interval;}

    SHDposHostcapability& m_host;
    std::set<p2p::NodeID> peers;
    /// config status
    std::map<h256, merkleState> m_requestStatus;
    std::map<p2p::NodeID, h256> m_nodesStatus;
    /// request status.
    std::map<p2p::NodeID, configState> m_unconfig;


    SHDposSyncState m_state = SHDposSyncState::Idle;

    /// temp blocks for import on complte sync.
    std::map<h256, bytes> m_blocks;
    std::map<uint64_t, h256> m_number_hash;
    BlockHeader         m_latestImportBlock;


    /// h256 block hash
    /// @param std::pair<bytes, uint64_t>
    ///         uint64_t: block time.  use for remove
    std::map<h256, uint64_t> m_know_blocks_hash;

    /// h256 transaction hash
    /// @param std::pair<bytes, uint64_t>
    ///         bytes : transaction data.
    ///         uint64_t: block time.  use for remove
    std::map<h256, uint64_t> m_know_transactionHash;

    int64_t  m_block_interval = 0;
};
}  // namespace brc
}  // namespace dev