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

	size_t badBlockNum_nextEpochDelete = 3;  // bad block num for will delete varlitor next epoch
	size_t badBlockNum_cantNotCreateBlock = 7;  // bad block num for cant't create block  cantNotCreateNum num;
	size_t cantNotCreateNum = 100;            // cant't create num of block
	size_t badBlockNum_deleteVarlitorAtOnce = 12; // if the bad block have this num delete Varlitor at now

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

/***************************网络数据包 封装业务数据 start*********************************************/
struct SHDposMsgPacket
{
	h512        node_id;     // peer id
	unsigned    packet_id;   //msg id
	bytes       data;        //rlp data
	SHDposMsgPacket() :node_id(h512(0)), packet_id(0) { }
	SHDposMsgPacket(h512 _id, unsigned _pid, bytesConstRef _data)
		:node_id(_id), packet_id(_pid), data(_data.toBytes())
	{
	}
};

struct SHDPosStatusMsg
{
	Address             m_addr;
	SHDPosStatusMsg() : m_addr(Address()) { }
	virtual void streamRLPFields(RLPStream& _s)
	{
		_s << m_addr.asBytes();
	}
	virtual void populate(RLP const& _rlp)
	{
		size_t field = 0;
		try
		{
			m_addr = Address(_rlp[field = 0].toBytes());
		}
		catch(Exception const& /*_e*/)
		{
			/*_e << errinfo_name("invalid msg format") << BadFieldError(field, toHex(_rlp[field].data().toBytes()));
			throw;*/
			cwarn << "populate DPosStatusMsg is error feild :" << field;
		}
	}

	friend std::ostream& operator << (std::ostream& out, SHDPosStatusMsg const& _d)
	{
		out << "{" <<
			std::endl << "Address:" << _d.m_addr;
		return out;
	}
};

using SHDposMsgQueue = dev::concurrent_queue<SHDposMsgPacket>;

/***************************网络数据包 封装业务数据 start*********************************************/
}
}
