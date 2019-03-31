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
#include "DposData.h"

namespace dev
{
namespace bacd
{
using NodeID = p2p::NodeID;

enum DposPacketType :byte
{
	DposStatuspacket = 0x23,
	SHDposDataPacket,
	SHDposBadBlockPacket,
	DposPacketCount
};

struct DposConfigParams
{
	size_t epochInterval = 6000;         //  sh-dpos a epoch need time ms
	size_t varlitorInterval = 1000;      //  a varlitor to create block time ms     
	size_t blockInterval = 1000;         //  a block created need time ms
	size_t valitorNum = 2;               //  check the varlitor for min num 
	size_t maxValitorNum = 21;           //  check the varlitor for max nu
	size_t verifyVoteNum = 6;            //  the vote chencked about block num 
	bool   isGensisVarNext = false;      
	u256   candidateBlance = 1000000;    // to be candidatator need brc
	void operator = (DposConfigParams const& _f)
	{
		epochInterval = _f.epochInterval;
		varlitorInterval = _f.varlitorInterval;
		blockInterval = _f.blockInterval;
		valitorNum = _f.valitorNum;
		maxValitorNum = _f.maxValitorNum;
		verifyVoteNum = _f.verifyVoteNum;
		isGensisVarNext = _f.isGensisVarNext;
	}
	friend std::ostream& operator << (std::ostream& out, DposConfigParams const& _f)
	{
		out << "epochInterval:" << _f.epochInterval << "ms|varlitorInterval:" << _f.varlitorInterval <<
			"ms|blockInterval:" << _f.blockInterval << "ms|checkvarlitorNum:" << _f.valitorNum <<
			"|maxValitorNum:" << _f.maxValitorNum << "|verifyVoteNum:" << _f.verifyVoteNum <<
			"| isGensisVarNext:" << _f.isGensisVarNext;
		return out;
	}
};

struct BadBlocks
{
	std::map<dev::h256, bytes>  m_badBlock;     // badBlock 
	BadBlocks()
	{
		m_badBlock.clear();
	}
	bool findbadblock(dev::h256 const& _h256)
	{
		if(m_badBlock.find(_h256) == m_badBlock.end())
			return false;
		return true;
	}
    bool insert(dev::h256 _hash, bytes const& _b)
	{
		if(findbadblock(_hash))
			return false;
		m_badBlock[_hash] = _b;
		return true;
	}
};
struct BadVarlitors
{
	size_t  m_epoch;
	std::map<Address, size_t>   m_badVarlitors;
	BadVarlitors() : m_epoch(0)
	{
		m_badVarlitors.clear();
	}
	inline bool isEpoch(size_t _e) { return m_epoch == _e; }
	size_t getBadBlockNum(Address const& _addr)
	{
		auto ret = m_badVarlitors.find(_addr);
		if(ret == m_badVarlitors.end())
			return 0;
		return ret->second;
	}
	void resetInNewEpoch(size_t _e)
	{
		if(m_epoch >= _e)
			return;
		m_epoch = _e;
		m_badVarlitors.clear();
	}
    void addBadVarlitor(Address const& _addr)
	{
		auto ret = m_badVarlitors.find(_addr);
		if(ret == m_badVarlitors.end())
			m_badVarlitors[_addr] = 1;
		ret->second += 1;
	}
};


/***************************网络数据包 封装业务数据 start*********************************************/
struct DposMsgPacket
{
	h512        node_id;     // peer id
	unsigned    packet_id;   //msg id
	bytes       data;        //rlp data
	DposMsgPacket() :node_id(h512(0)), packet_id(0) { }
	DposMsgPacket(h512 _id, unsigned _pid, bytesConstRef _data)
		:node_id(_id), packet_id(_pid), data(_data.toBytes())
	{
	}
};

using DposMsgQueue = dev::concurrent_queue<DposMsgPacket>;

struct DPosStatusMsg
{
	Address             m_addr;
	DPosStatusMsg() : m_addr(Address()) { }
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

	friend std::ostream& operator << (std::ostream& out, DPosStatusMsg const& _d)
	{
		out << "{" <<
			std::endl << "Address:" << _d.m_addr;
		return out;
	}
};
struct SH_DposData :DPosStatusMsg
{


};

struct SH_DposBadBlock :DPosStatusMsg
{
	dev::SignatureStruct  m_signData;
	bytes               m_badBlock;
	SH_DposBadBlock() :DPosStatusMsg()
	{
		m_signData = SignatureStruct();
		m_badBlock = bytes();

	}
	virtual void streamRLPFields(RLPStream& _s) override;
	virtual void populate(RLP const& _rlp) override;

	friend std::ostream& operator << (std::ostream& out, SH_DposBadBlock const& _d)
	{
		out << "{" <<
			std::endl << "Address:" << _d.m_addr << "bad msg";
		return out;
	}
};


/***************************网络数据包 封装业务数据 start*********************************************/
}
}