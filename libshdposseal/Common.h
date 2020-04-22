#pragma once
/*
    dpos 相关数据结构
*/
#include <iostream>
#include <libdevcore/concurrent_queue.h>
#include <libdevcore/RLP.h>
#include <libdevcrypto/Common.h>
#include <libbrccore/Exceptions.h>
#include <libp2p/Common.h>
#include <libdevcore/FixedHash.h>
#include <libbrccore/Common.h>
#include <libdevcore/Log.h>
#include <libbrcdchain/Block.h>

namespace dev
{
namespace brc
{
using NodeID = p2p::NodeID;

enum SHDposPacketType : unsigned
{
	SHDposStatuspacket = 0x1,
	SHDposPacketCount
};

enum CreaterType
{
	Varlitor = 0,
	Canlitor
};

struct SHDposConfigParams
{
	size_t epochInterval = 0;         //  sh-dpos a epoch need time ms
	size_t varlitorInterval = 1000;      //  a varlitor to create block time ms     
	size_t blockInterval = 1000;         //  a block created need time ms
    size_t totalElectorNum = 51;
	u256   candidateBlance = 1000000;    // to be candidatator need brc


	void operator = (SHDposConfigParams const& _f)
	{
		epochInterval = _f.epochInterval;
		varlitorInterval = _f.varlitorInterval;
		blockInterval = _f.blockInterval;
	}
	friend std::ostream& operator << (std::ostream& out, SHDposConfigParams const& _f) 
	{
		out << "epochInterval:" << _f.epochInterval << "ms|varlitorInterval:" << _f.varlitorInterval <<
			"ms|blockInterval:" << _f.blockInterval;
		return out;
	}
};
}
}
