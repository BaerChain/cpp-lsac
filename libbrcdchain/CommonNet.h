#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libp2p/Common.h>
#include <chrono>
#include <string>

namespace dev
{

class OverlayDB;

namespace brc
{

static const unsigned c_maxHeaders = 2048;      ///< Maximum number of hashes BlockHashes will ever send.
static const unsigned c_maxHeadersAsk = 2048;   ///< Maximum number of hashes GetBlockHashes will ever ask for.
static const unsigned c_maxBlocks = 128;        ///< Maximum number of blocks Blocks will ever send.
static const unsigned c_maxBlocksAsk = 128;     ///< Maximum number of blocks we ask to receive in Blocks (when using GetChain).
static const unsigned c_maxPayload = 262144;    ///< Maximum size of packet for us to send.
static const unsigned c_maxNodes = c_maxBlocks; ///< Maximum number of nodes will ever send.
static const unsigned c_maxReceipts = c_maxBlocks; ///< Maximum number of receipts will ever send.

class BlockChain;
class TransactionQueue;
class BrcdChainCapability;
class BrcdChainPeer;

enum SubprotocolPacketType: byte
{
    StatusPacket = 0x00,
    NewBlockHashesPacket = 0x01,
    TransactionsPacket = 0x02,
    GetBlockHeadersPacket = 0x03,
    BlockHeadersPacket = 0x04,
    GetBlockBodiesPacket = 0x05,
    BlockBodiesPacket = 0x06,
    NewBlockPacket = 0x07,

    GetNodeDataPacket = 0x0d,
    NodeDataPacket = 0x0e,
    GetReceiptsPacket = 0x0f,
    ReceiptsPacket = 0x10,
    GetLatestStatus = 0x11,
    UpdateStatus = 0x12,
    PacketCount
};

enum class Asking
{
    State,
    BlockHeaders,
    BlockBodies,
    NodeData,
    Receipts,
    WarpManifest,
    WarpData,
    UpdateStatus,
    Nothing
};

enum class SyncState
{
    NotSynced,          ///< Initial chain sync has not started yet
    Idle,               ///< Initial chain sync complete. Waiting for new packets
    Waiting,            ///< Block downloading paused. Waiting for block queue to process blocks and free space
    Blocks,             ///< Downloading blocks
    State,              ///< Downloading state
    Size        /// Must be kept last
};

struct SyncStatus
{
    SyncState state = SyncState::Idle;
    unsigned protocolVersion = 0;
    unsigned startBlockNumber;
    unsigned currentBlockNumber;
    unsigned highestBlockNumber;
    bool majorSyncing = false;
};

using NodeID = p2p::NodeID;
}
}
