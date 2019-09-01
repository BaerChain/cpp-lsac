#include "BrcdChainPeer.h"
#include <libbrccore/Common.h>
#include <libp2p/CapabilityHost.h>


using namespace std;
using namespace dev;
using namespace dev::brc;

static std::string const c_brcCapability = "brc";

namespace
{
string toString(Asking _a)
{
    switch (_a)
    {
    case Asking::BlockHeaders:
        return "BlockHeaders";
    case Asking::BlockBodies:
        return "BlockBodies";
    case Asking::NodeData:
        return "NodeData";
    case Asking::Receipts:
        return "Receipts";
    case Asking::Nothing:
        return "Nothing";
    case Asking::State:
        return "State";
    case Asking::WarpManifest:
        return "WarpManifest";
    case Asking::WarpData:
        return "WarpData";
    case Asking::UpdateStatus:
        return "UpdateStatus";
    }
    return "?";
}
}  // namespace

void BrcdChainPeer::setStatus(unsigned _protocolVersion, u256 const& _networkId,
    u256 const& _totalDifficulty, h256 const& _latestHash, h256 const& _genesisHash, u256 const &height)
{
    m_protocolVersion = _protocolVersion;
    m_networkId = _networkId;
    m_totalDifficulty = _totalDifficulty;
    m_latestHash = _latestHash;
    m_genesisHash = _genesisHash;
    m_height = height;
}


std::string BrcdChainPeer::validate(
    h256 const& _hostGenesisHash, unsigned _hostProtocolVersion, u256 const& _hostNetworkId) const
{
    std::string error;
    if (m_genesisHash != _hostGenesisHash)
        error = "Invalid genesis hash.";
    else if (m_protocolVersion != _hostProtocolVersion)
        error = "Invalid protocol version.";
    else if (m_networkId != _hostNetworkId)
        error = "Invalid network identifier.";
    else if (m_asking != Asking::State && m_asking != Asking::Nothing)
        error = "Peer banned for unexpected status message.";

    return error;
}

void BrcdChainPeer::requestStatus(
    u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash, u256 height)
{
    assert(m_asking == Asking::Nothing);
    setAsking(Asking::State);
    m_requireTransactions = true;
    RLPStream s;
    m_host->prep(m_id, c_brcCapability, s, StatusPacket, 6)
        << c_protocolVersion << _hostNetworkId << _chainTotalDifficulty << _chainCurrentHash
        << _chainGenesisHash << height;
    m_host->sealAndSend(m_id, s);
}

void BrcdChainPeer::requestLatestStatus(){

    RLPStream s;
    setAsking(Asking::UpdateStatus);
    LOG(m_logger) << "requestLatestStatus  " << ::toString(m_asking) << "  m_id: " << m_id;
    m_host->prep(m_id, c_brcCapability, s, GetLatestStatus, 0);
    m_host->sealAndSend(m_id, s);
}

void BrcdChainPeer::requestBlockHeaders(
    unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse)
{
    if (m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking headers while requesting " << ::toString(m_asking);
    }
    setAsking(Asking::BlockHeaders);
    RLPStream s;
    m_host->prep(m_id, c_brcCapability, s, GetBlockHeadersPacket, 4)
        << _startNumber << _count << _skip << (_reverse ? 1 : 0);
    LOG(m_logger) << "Requesting " << _count << " block headers starting from " << _startNumber
                  << (_reverse ? " in reverse" : "");
    m_lastAskedHeaders = _count;
    m_host->sealAndSend(m_id, s);
}

void BrcdChainPeer::requestBlockHeaders(
    h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse)
{
    if (m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking headers while requesting " << ::toString(m_asking);
    }
    setAsking(Asking::BlockHeaders);
    RLPStream s;
    m_host->prep(m_id, c_brcCapability, s, GetBlockHeadersPacket, 4)
        << _startHash << _count << _skip << (_reverse ? 1 : 0);
    LOG(m_logger) << "Requesting " << _count << " block headers starting from " << _startHash
                  << (_reverse ? " in reverse" : "");
    m_lastAskedHeaders = _count;
    m_host->sealAndSend(m_id, s);
}


void BrcdChainPeer::requestBlockBodies(h256s const& _blocks)
{
    requestByHashes(_blocks, Asking::BlockBodies, GetBlockBodiesPacket);
}

void BrcdChainPeer::requestNodeData(h256s const& _hashes)
{
    requestByHashes(_hashes, Asking::NodeData, GetNodeDataPacket);
}

void BrcdChainPeer::requestReceipts(h256s const& _blocks)
{
    requestByHashes(_blocks, Asking::Receipts, GetReceiptsPacket);
}

void BrcdChainPeer::requestByHashes(
    h256s const& _hashes, Asking _asking, SubprotocolPacketType _packetType)
{
    if (m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking " << ::toString(_asking) << " while requesting "
                      << ::toString(m_asking);
    }
    setAsking(_asking);
    if (_hashes.size())
    {
        RLPStream s;
        m_host->prep(m_id, c_brcCapability, s, _packetType, _hashes.size());
        for (auto const& i : _hashes)
            s << i;
        m_host->sealAndSend(m_id, s);
    }
    else
        setAsking(Asking::Nothing);
}
