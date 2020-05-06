#pragma once

#include <libp2p/Common.h>
#include <string>
namespace dev
{
namespace brc
{
class SHDposHostcapability;


struct merkleState
{
    std::vector<uint64_t> blocks;
    h256 merkleHash;
    std::vector<p2p::NodeID> nodes;
    uint64_t m_latestRequest = 0;


    bool isNull() const {
        return blocks.size() == 0 
        && merkleHash == h256() 
        && nodes.size() == 0
        && m_latestRequest == 0;
    }
};

struct configState{
    p2p::NodeID id;
    std::vector<std::vector<uint64_t>> request_blocks;
};



class SHDposSync
{
public:
    explicit SHDposSync(SHDposHostcapability& host);

    void addNode(const p2p::NodeID& id);
    void configNode(const p2p::NodeID& id, const RLP& data);
    void getBlocks(const p2p::NodeID& id, const RLP& data);
    void blockHeaders(const p2p::NodeID& id, const RLP& data);
    void newBlocks(const p2p::NodeID& id, const RLP& data);

private:
    void continueSync(const p2p::NodeID& id);

    SHDposHostcapability& m_host;


    uint32_t m_lastRequestNumber = 0;
    h256 m_lasttRequestHash;
    uint32_t m_lastImportedNumber = 0;

    ///
    std::map<p2p::NodeID, std::map<uint64_t, u256>> m_sendBlocks;

    // Block numbers , block hash
    std::map<uint64_t, h256> m_recevieBlocks;

    std::set<p2p::NodeID> peers;

    /// config status
    std::map<h256, merkleState> m_requestStatus;

    /// request status.
    std::map<p2p::NodeID, configState> m_unconfig;
};
}  // namespace brc
}  // namespace dev