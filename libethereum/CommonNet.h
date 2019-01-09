/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file CommonNet.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * Miscellanea required for the PeerServer/Session classes.
 */

#pragma once

#include <string>
#include <chrono>
#include <libdevcore/Common.h>
#include <libdevcore/Log.h>

namespace dev
{

class OverlayDB;

namespace eth
{

#if ETH_DEBUG
static const unsigned c_maxHashes = 2048;		///< Maximum number of hashes BlockHashes will ever send.
static const unsigned c_maxHashesAsk = 2048;		///< Maximum number of hashes GetBlockHashes will ever ask for.
static const unsigned c_maxBlocks = 128;		///< Maximum number of blocks Blocks will ever send.
static const unsigned c_maxBlocksAsk = 128;		///< Maximum number of blocks we ask to receive in Blocks (when using GetChain).
static const unsigned c_maxPayload = 262144;	///< Maximum size of packet for us to send.
#else
static const unsigned c_maxHashes = 2048;		///< Maximum number of hashes BlockHashes will ever send.
static const unsigned c_maxHashesAsk = 2048;	///< Maximum number of hashes GetBlockHashes will ever ask for.
static const unsigned c_maxBlocks = 128;		///< Maximum number of blocks Blocks will ever send.
static const unsigned c_maxBlocksAsk = 128;		///< Maximum number of blocks we ask to receive in Blocks (when using GetChain).
static const unsigned c_maxPayload = 262144;	///< Maximum size of packet for us to send.
#endif

class BlockChain;
class TransactionQueue;
class EthereumHost;
class EthereumPeer;

enum
{
	StatusPacket = 0,
	NewBlockHashesPacket,
	TransactionsPacket,
	GetBlockHashesPacket,
	BlockHashesPacket,
	GetBlocksPacket,
	BlocksPacket,
	NewBlockPacket,
	GetBlockHashesByNumberPacket,
	PacketCount
};

enum class Asking
{
	State,
	Hashes,
	Blocks,
	Nothing
};

enum class SyncState
{
	Idle,				///< Initial chain sync complete. Waiting for new packets
	WaitingQueue,		///< Block downloading paused. Waiting for block queue to process blocks and free space
	HashesNegotiate,	///< Waiting for first hashes to arrive
	HashesSingle,		///< Locked on and downloading hashes from a single peer
	HashesParallel,		///< Downloading hashes from multiple peers over
	Blocks,				///< Downloading blocks
	NewBlocks,			///< Downloading blocks learned from NewHashes packet

	Size		/// Must be kept last
};

struct SyncStatus
{
	SyncState state = SyncState::Idle;
	unsigned hashesTotal = 0;
	unsigned hashesReceived = 0;
	bool hashesEstimated = false;
	unsigned blocksTotal = 0;
	unsigned blocksReceived = 0;
};

}
}