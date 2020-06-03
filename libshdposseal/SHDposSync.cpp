#include "SHDposSync.hpp"

#include "SHDposHostCapability.h"
#include "libbrcdchain/TransactionQueue.h"

static const uint64_t MAX_REQUEST_BLOKCS = 128;
static const uint64_t MAX_TEMP_SYNC_BLOCKS = 1000;

namespace dev
{
namespace brc
{
SHDposSync::SHDposSync(SHDposHostcapability& host) : m_host(host), m_state(SHDposSyncState::Sync)
{
    m_latestImportBlock = m_host.chain().info();
    CP2P_LOG << "read from blockchain " << m_latestImportBlock.hash() << m_latestImportBlock.number();
}


void SHDposSync::addNode(const p2p::NodeID& id)
{
    peers.insert(id);
    // config same chain.
    if (0 == m_requestStatus.size())
    {
        auto node_peer = m_host.getNodePeer(id);
        CP2P_LOG << "Add node ......id : " << id << " height : " << node_peer.getHeight() << "c";
        auto request_blocks = getRequestHeights(node_peer.getHeight());
        configState cs;
        cs.id = id;
        cs.height = node_peer.getHeight();
        cs.request_blocks = request_blocks;
        m_unconfig[id] = cs;

        node_peer.requestNodeConfig(request_blocks);
    }
    else
    {
        ///has old requestStatus and will resuest all old_nodeStatus
        CP2P_LOG << " m_requestStatus.size:"<< m_requestStatus.size() << "nodeId:"<< id;
        auto & cs = m_unconfig[id];
        cs.id = id;
        for(auto & r : m_requestStatus){
            cs.un_hashs.insert(r.first);
        }
        requestOldStatus(id);
    }
}

std::vector<uint64_t > SHDposSync::getRequestHeights(uint64_t _height){
    std::vector<uint64_t> request_blocks;
    uint64_t height = _height;
    if (12 >= height)
    {
        request_blocks.emplace_back(0);
    }
    else
    {
        /// m_lastImportedNumber, 1/4, 1/2, 3/4, height -12.
        request_blocks.emplace_back(m_latestImportBlock.number() <= height ? m_latestImportBlock.number() : 1);
        height -= 12;
        request_blocks.emplace_back(height / 4);
        request_blocks.emplace_back(height / 2);
        request_blocks.emplace_back(height * 3 / 4);
        request_blocks.emplace_back(height);
    }
    return request_blocks;
}

void SHDposSync::requestOldStatus(const p2p::NodeID& id) {
    if(!m_unconfig.count(id)) {
        CP2P_LOG << " not has requestStatus...";
        return;
    }
    auto &cs = m_unconfig[id];
    if(cs.un_hashs.empty()){
        CP2P_LOG << "all  requestStatus   is resuested...";
        m_unconfig.erase(id);
        return;
    }
    auto begin = cs.un_hashs.begin();
    cs.request_hash = *begin;
    cs.un_hashs.erase(begin);
    auto request = m_requestStatus[cs.request_hash];
    if( request.nodes.count(id)){
        CP2P_LOG << " this node is already and skip...";
        requestOldStatus(id);
        return;
    }

    CP2P_LOG << "will resuest node:"<< id << " height:"<< request.fristRequestNumber;
    auto node_peer = m_host.getNodePeer(id);
    auto request_blocks = getRequestHeights(node_peer.getHeight());
    cs.request_blocks = request_blocks;
    node_peer.requestNodeConfig(request_blocks);
}
void SHDposSync::updateUnconfig(const p2p::NodeID& id){
    if(!m_unconfig.count(id)){
        return;
    }
    if(!m_unconfig[id].un_hashs.empty())
        return;
    m_unconfig.erase(id);
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
        CP2P_LOG << " number:" << b.number() << " hash"<< b.hash();
    }
    // import, 1/4, 2/4, 3/4, height
    // config common hash
    if ((blockinfo[unconfig.request_blocks[0]].hash() == m_host.chain().numberHash(unconfig.request_blocks[0])) ||
                                                        m_latestImportBlock.number() == 0)
    {
        CP2P_LOG << "m_latestImportBlock.number() | unconfig.height):" << m_latestImportBlock.number()<<" | "<<unconfig.height;
        if(m_latestImportBlock.number()  >= unconfig.height){
            // caculate merkle hash.
            //m_unconfig.erase(id);
            updateUnconfig(id);
            CP2P_LOG << " config success...";
            return true;
        }
        // set the sync state
        m_state = SHDposSyncState::Sync;
        /// TODO m_requestStatus.size() ==0 or !=0
        if(unconfig.request_hash == h256())
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

            ms.latestRequestNumber = m_latestImportBlock.number();
            if (blockinfo[unconfig.request_blocks[0]].number() != 1)
            {
                ms.latestImportBlock = blockinfo[unconfig.request_blocks[0]];
            }

            ms.updateMerkleHash();

            // caculate merkle hash.
            ms.fristRequestNumber = m_unconfig[id].height;
            m_unconfig.erase(id);
            CP2P_LOG << " config success..." << " state: SYNC...";
            m_nodesStatus[id] = ms.merkleHash;
            m_requestStatus[ms.merkleHash] = ms;
            ms.nodes.insert(id);

            continueSync(id);
        }
        else{
            // has more requestStatus
            auto _hash = unconfig.request_hash;
            unconfig.request_hash = h256();

            requestOldStatus(id);
        }
    }
    else
    {
        // find common hash.
        m_unconfig.erase(id);
        BlockHeader h (blocks.back());
        CP2P_LOG << "not find lastImportHash... request number:"<<h.number()<< " hashs :" << h.hash();
        std::vector<h256> hashs = {h.hash()};
        m_host.getNodePeer(id).requestBlocks(hashs);
    }

    return true;
}
std::vector<bytes> SHDposSync::getBlocks( const RLP& data){
    std::vector<bytes> blocks;
    size_t itemCount = data.itemCount();
    if (itemCount != 2)
    {
        return blocks;
    }
    /// get block by number
    if (1 == data[0].toInt())
    {
        std::vector<uint64_t> numbers = data[1].toVector<uint64_t>();

        CP2P_LOG << "get blocks number " << " data size " << numbers.size();
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
        CP2P_LOG << "get blocks hash " << " data size " << hashs;
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
    return blocks;
}
void SHDposSync::getBlocks(const p2p::NodeID& id, const RLP& data)
{
    auto blocks = getBlocks(data);
    if(blocks.empty())
        return;
    CP2P_LOG << "get blocks :"<< id;
    m_host.getNodePeer(id).sendBlocks(blocks);
}

void SHDposSync::getConfigBlocks(const p2p::NodeID& id, const RLP& data){
    auto blocks = getBlocks(data);
    CP2P_LOG << " getConfigBlocks id:"<< id;
    m_host.getNodePeer(id).sendConfigBlocks(blocks);
}

h256 SHDposSync::collectBlockSync(const p2p::NodeID& id, const RLP& data)
{
    auto& hash = m_nodesStatus[id];
    auto& requestState = m_requestStatus[hash];     // if not has value return null_struct
    requestState.latestImportBlock = m_host.chain().info();
    requestState.latestConfigNumber = std::max(requestState.latestConfigNumber, (uint64_t)requestState.latestImportBlock.number());
    requestState.latestImportNumber = requestState.latestImportBlock.number();

    std::vector<bytes> blocks = data[0].toVector<bytes>();
    h256 late_hash = requestState.latestImportBlock.hash();
    CP2P_LOG << "late_hash " << late_hash;
    if(blocks.empty()){
        CP2P_LOG << "get the blocks data is empty";
        return hash;
    }
    /// this blocks is continuous blocks, and will check the frist hash
    for (size_t i = 0; i < blocks.size(); i++)
    {
        BlockHeader b(blocks[i]);
        if(m_host.chain().isKnown(b.hash())){
            requestState.latestConfigNumber = b.number();
            continue;
        }
        if (b.number() == 1 || late_hash == b.parentHash() || m_host.chain().isKnown(b.parentHash()))
        {
            CP2P_LOG << "config " << b.number() << " hash " << b.hash() << " parent "
                     << b.parentHash() << " late_hash " << late_hash;
            late_hash = b.hash();
            requestState.cacheBlocks[b.number()] = blocks[i];
            requestState.latestConfigNumber = b.number();
            requestState.latestImportNumber = std::min(requestState.latestImportNumber, (uint64_t)b.number()-1);
            m_host.getNodePeer(id).makeBlockKonw(b.hash());
            continue;
        }
        else{
            /// resuest parent_block
            CP2P_LOG << "request parent Number " << b.number()-1;
            requestState.latestRequestNumber = b.number()-1;
            std::vector<uint64_t> requestNumber={(uint64_t)b.number()-1};
            m_host.getNodePeer(id).requestBlocks(requestNumber);
            return hash;
        }
    }


    BlockHeader end_block(blocks.back());
    if (end_block.number() != requestState.latestConfigNumber)
    {
        CP2P_LOG << "find receive block end " << end_block.number() << " hash " << end_block.hash()
                 << " config: " << requestState.latestConfigNumber << "  start "
                 << BlockHeader(blocks.front()).number();
        std::vector<uint64_t> requestNumber;
        uint64_t re_end = std::min( requestState.latestConfigNumber + MAX_REQUEST_BLOKCS, (uint64_t)end_block.number());

        for (uint64_t start = requestState.latestConfigNumber; start <= re_end; start++)
        {
            requestNumber.push_back(start);
        }
        CP2P_LOG << "requestNumber " << requestNumber;
        requestState.latestRequestNumber = re_end;
        m_host.getNodePeer(id).requestBlocks(requestNumber);
    }

    return hash;
}

void SHDposSync::blockHeaders(const p2p::NodeID& id, const RLP& data)
{
    CP2P_LOG << " state:" << (int) m_state << " uconfig:" << m_unconfig.count(id);
    //if (SHDposSyncState::Idle == m_state && !m_unconfig.count(id))
    if (SHDposSyncState::Idle == m_state)
    {
        processBlock(id, data);
        CP2P_LOG << " process end .....";
    }
    else
    {
        syncBlock(id, data);
    }
    //clearTemp();
}

void SHDposSync::processBlock(const p2p::NodeID& id, const RLP& data)
{
    std::vector<bytes> blocks = data[0].toVector<bytes>();
    CP2P_LOG << "processBlock " << blocks.size();
    for (auto itr : blocks)
    {
        try
        {
            BlockHeader bh(itr);
            m_host.getNodePeer(id).makeBlockKonw(bh.hash());
            // if the block is imported continue
            if(m_latestImportBlock.number() <=bh.number() && m_host.chain().isKnown(bh.hash())){
                continue;
            }
            m_number_hash[bh.number()] = bh.hash();
            m_blocks[bh.hash()] = itr;
        }
        catch (const std::exception& e)
        {
            CP2P_LOG << "exception " << e.what();
            return;
        }
    }
    CP2P_LOG << "number_hash" << m_number_hash;

    auto begin = m_number_hash.begin();

    auto requestParent = [&](h256 const& _h){
        std::vector<h256> hashs;
        hashs.push_back(_h);
        CP2P_LOG << "request hashs :" << hashs;
        m_host.getNodePeer(id).requestBlocks(hashs);
    };

    if (begin != m_number_hash.end())
    {
        BlockHeader bh(m_blocks[begin->second]);
        CP2P_LOG << "m_lastImportedNumber: " << m_latestImportBlock.number() << " : " << bh.number();
        if (m_latestImportBlock.number() + 1 == bh.number() || m_host.chain().isKnown(bh.parentHash()))
        {
            // import block.
            CP2P_LOG << "will import block ";
            for (auto& itr : m_number_hash)
            {
                if (m_blocks.count(itr.second))
                {
                    CP2P_LOG << "will import block " << itr.second;
                    auto import_result = m_host.bq().import(&m_blocks[itr.second]);
                    switch (import_result)
                    {
                        case ImportResult::Success: {
                            //m_latestImportBlock = BlockHeader(data);
                            break;
                        }
                        case ImportResult::AlreadyInChain: {
                            CP2P_LOG << "AlreadyInChain";
                            break;
                        }
                        case ImportResult::UnknownParent: {
                            CP2P_LOG << "UnknownParent";
                            clearTemp();
                            BlockHeader h(m_blocks[itr.second]);
                            requestParent(h.parentHash());
                            return;
                        }
                        case ImportResult::FutureTimeKnown: {
                            CP2P_LOG << "FutureTimeKnown";
                        }
                        case ImportResult::FutureTimeUnknown: {
                            CP2P_LOG << "FutureTimeUnknown";
                        }
                        case ImportResult::AlreadyKnown: {
                            CP2P_LOG << "AlreadyKnown ";
                        }
                        case ImportResult::Malformed: {
                            CP2P_LOG << "Malformed";
                        }
                        case ImportResult::OverbidGasPrice: {
                            CP2P_LOG << "OverbidGasPrice";
                        }
                        case ImportResult::BadChain: {
                        }
                        case ImportResult::ZeroSignature: {
                        }
                        case ImportResult::NonceRepeat: {
                        }
                        default: {
                            CP2P_LOG << "unkown import block result.";
                            clearTemp();
                            return;
                        }
                    }
                    m_requestStatus[m_nodesStatus[id]].latestImportBlock = m_latestImportBlock;
                }
                else
                {
                    cwarn << "warning, the cash block is miss.....";
                }
            }
           clearTemp();
        }
        else
        {
            // request block , find parent block.
            auto parent_bytes = m_host.chain().block(begin->second);
            if (parent_bytes.size() == 0)
            {
                /// go to get parent.
                requestParent(bh.parentHash());
            }
        }
    }
    else
    {
        CP2P_LOG << "warning , get m_number_hash.size() == 0";
    }
}

void SHDposSync::syncBlock(const p2p::NodeID& id, const RLP& data)
{
    /// config node state
//    if (m_unconfig.count(id))
//    {
//        configNode(id, data);
//        return;
//    }
    auto merkleHash = collectBlockSync(id, data);

    auto& requestState = m_requestStatus[merkleHash];
    if (requestState.latestConfigNumber > requestState.latestImportNumber && requestState.cacheBlocks.size() > 0)
    {
        CP2P_LOG << "import blocks " << requestState.cacheBlocks.size() << " from "
                 << requestState.latestImportBlock.number()
                 << " to:" << requestState.latestConfigNumber;

        for (uint64_t start = requestState.latestImportNumber+1;start <= requestState.latestConfigNumber; start++)
        {
            if (!importedBlock(requestState.cacheBlocks[start]))
            {
                /// TODO
                cwarn << " imported field number:"<< start;
                CP2P_LOG << "imported block error. TODO";
                break;
            }
            else
            {
                requestState.latestImportBlock = BlockHeader(&requestState.cacheBlocks[start]);
            }
        }

        auto iter = requestState.cacheBlocks.begin();
        while (iter != requestState.cacheBlocks.end())
        {
            if (iter->first <= requestState.latestConfigNumber)
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
        if (!m_know_blocks_hash.count(itr))
        {
            unkownHash.push_back(itr);
        }
        m_host.getNodePeer(id).makeBlockKonw(itr);
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
    if (m_state == SHDposSyncState::Idle)
    {
        return;
    }
    auto& hash = m_nodesStatus[id];
    auto& requestState = m_requestStatus[hash];

    {
        BlockHeader bh = requestState.latestImportBlock;
        if (bh.timestamp() != -1)
        {
            //CP2P_LOG << "now " << utcTimeMilliSec() << " latest import " << bh.timestamp()<< " diff " << utcTimeMilliSec() - bh.timestamp();
            if (utcTimeMilliSec() - bh.timestamp() < getBlockInterval()*2)
            {
                completeSync();
                return;
            }
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

    auto peer_height = m_host.getNodePeer(id).getHeight();
    if (peer_height <= start_requrest)
    {
        //m_host.getNodePeer(id).requestState();
        return;
    }
    if (peer_height <= end_request)
    {
        end_request = peer_height;
    }

    std::vector<uint64_t> numbers;

    for (auto i = start_requrest; i <= end_request; i++)
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
        else
        {
            for (auto& itr : peers)
            {
                continueSync(itr);
            }
        }
    }
     else
     {
         CP2P_LOG << "ignore restart sync";
     }
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
    m_nodesStatus.clear();
    m_unconfig.clear();
    m_requestStatus.clear();
//    for (auto & n : m_requestStatus){
//        n.second.configBlocks.clear();
//        n.second.cacheBlocks.clear();
//    }

  
}


void SHDposSync::addKnowBlock(const std::vector<h256>& hash)
{
    for (auto& itr : hash)
    {
        m_know_blocks_hash[itr] = utcTimeMilliSec();
    }
}
void SHDposSync::addKnowTransaction(const std::vector<h256>& hash)
{
    for (auto& itr : hash)
    {
        m_know_transactionHash[itr] = utcTimeMilliSec();
    }
}

void SHDposSync::getTransaction(const p2p::NodeID& id, const RLP& data)
{
    std::vector<h256> hashs = data[0].toVector<h256>();

    h256Hash hh;
    for (auto& it : hashs)
    {
        hh.insert(it);
    }

    auto transactions = m_host.Tq().topTransactions(0, hh);

    std::vector<bytes> send_data;
    for (auto& itr : transactions)
    {
        RLPStream rs;
        itr.streamRLP(rs);
        send_data.push_back(rs.out());
    }

    m_host.getNodePeer(id).sendTransactionBody(send_data);
}

void SHDposSync::importedTransaction(const p2p::NodeID& id, const RLP& data)
{
    std::vector<bytes> body = data[0].toVector<bytes>();
}

void SHDposSync::clearTemp(uint64_t expire)
{
    auto now = utcTimeMilliSec();

    // remove blocks.
    {
        auto itr = m_know_blocks_hash.begin();
        while (itr != m_know_blocks_hash.end())
        {
            if (now - itr->second > expire)
            {
                itr = m_know_blocks_hash.erase(itr);
            }
            else
            {
                itr++;
            }
        }
    }

    // remove transaction
    {
        auto itr = m_know_transactionHash.begin();
        while (itr != m_know_transactionHash.end())
        {
            if (now - itr->second > expire)
            {
                itr = m_know_transactionHash.erase(itr);
            }
            else
            {
                itr++;
            }
        }
    }
    m_number_hash.clear();
//    {
//        auto itr = m_number_hash.begin();
//        while(itr != m_number_hash.end()){
//
//        }
//    }
}

void SHDposSync::removeNode(const p2p::NodeID& id)
{
    peers.erase(id);
    auto merkle = m_nodesStatus[id];
    auto state = m_requestStatus[merkle];


    state.nodes.erase(id);
    if (state.nodes.size() == 0)
    {
        CP2P_LOG << "remove m_requestStatus " << merkle;
        m_requestStatus.erase(merkle);
    }
    m_nodesStatus.erase(id);

    CP2P_LOG << "remove m_requestStatus " << m_requestStatus.size();
    CP2P_LOG << "remove m_requestStatus " << m_nodesStatus.size();
}

bool SHDposSync::importedBlock(const bytes& data)
{
    auto import_result = m_host.bq().import(&data);
    switch (import_result)
    {
    case ImportResult::Success: {
        //m_latestImportBlock = BlockHeader(data);
        return true;
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
        return true;
        //break;
    }
    case ImportResult::AlreadyKnown: {
        CP2P_LOG << "AlreadyKnown ";
        //return true;
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
    return false;
}

bool SHDposSync::isKnowInChain(h256 const& _hash) const {
    auto header = m_host.chain().block(_hash);
    return !header.empty();
}

}  // namespace brc
}  // namespace dev