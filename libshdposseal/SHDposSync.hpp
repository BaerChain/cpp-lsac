#pragma once

#include <string>
#include <libp2p/Common.h>
namespace dev
{
namespace brc
{
class SHDposHostcapability;








class SHDposSync
{
public:
    explicit SHDposSync(SHDposHostcapability& host);

    
    void addNode(p2p::NodeID id);
    void getBlocks(p2p::NodeID id, const RLP &data);
    void BlockHeaders(p2p::NodeID id, const RLP &data);
    void NewBlocks(p2p::NodeID id, const RLP & data);
private:
    SHDposHostcapability& m_host;


    uint32_t m_lastRequestNumber = 0;
    h256     m_lasttRequestHash;
    uint32_t m_lastImportedNumber = 0;

    /// 
    std::map<p2p::NodeID, std::map<uint64_t, u256>> m_sendBlocks;

    //Block numbers , block hash
    std::map<uint64_t, h256> m_recevieBlocks;

    std::set<p2p::NodeID>  peers;


};
}  // namespace brc
}  // namespace dev