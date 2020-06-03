#pragma once
#include "Common.h"
#include "CommonNet.h"
#include <libp2p/CapabilityHost.h>
namespace dev
{
namespace brc
{




struct ProtocolData{
  
};





class NodePeer
{
public:
    NodePeer() {}
    NodePeer(std::shared_ptr<p2p::CapabilityHostFace> _host, const NodeID& _peerID)
      : m_host(_host), m_id(_peerID)
    {}

    void sendNewStatus(u256 height, h256 genesisHash, h256 latestBlock, uint32_t version);
    void setPeerStatus(u256 height, h256 genesisHash, h256 latestBlock, uint32_t version);


    uint64_t getHeight() const;
    h256 getLatestBlock() const;
    uint32_t getVersion() const;

    /*
    @param ids  block number or hash.
    */
    void requestBlocks(const std::vector<uint64_t>& ids);
    void requestBlocks(const std::vector<h256>& ids);

    /*
     meaasge for nodes connection and check
    @ param ids
    */
    void requestNodeConfig(const std::vector<uint64_t>& ids);
    void sendConfigBlocks(const std::vector<bytes>& blocks);
    /*
    @param hash   transaction hash.
    */
    void sendTransactionHashs(const std::vector<h256> &hashs);
    void sendTransactionBody(const std::vector<bytes> &data);

    /*
    @param
    */
    void sendBlocksHashs(const std::vector<h256>& hashs);

    /*
    @param ids blocks number
    */
    // void requestBlocksHash(const std::vector<uint64_t> &ids);

    void sendBlocks(const std::vector<bytes>& blocks);

    void requestState();
    void sendState(u256 height, h256 genesisHash, h256 latestBlock, uint32_t version);


    SHposAsking getAksing() const { return m_ask; }
    void completeAsk() { m_ask = SHposAsking::Nothing; }

    bool knowBlock(h256 const& _hash) const { return m_know_block_hash.count(_hash);}
    void makeBlockKonw(h256 const& _hash){ m_know_block_hash.insert(_hash);}

private:
    // only for send message. check state.
    // if Request return true, else false
    bool checkRequest() const;
    std::shared_ptr<p2p::CapabilityHostFace> m_host;
    NodeID m_id;

    // peer max height.
    uint64_t m_height;
    h256 m_genesisHash;
    h256 m_latestBlock;
    uint32_t m_version;
    // block hash.
    h256 latestBlock;
    SHposAsking m_ask = SHposAsking::Nothing;

    std::set<h256> m_know_block_hash;
};



}  // namespace brc
}  // namespace dev