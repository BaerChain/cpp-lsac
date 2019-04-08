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
namespace bacd
{
using NodeID = p2p::NodeID;

enum SHDposPacketType :byte
{
	SHDposStatuspacket = 0x23,
	SHDposDataPacket,
	SHDposBadBlockPacket,
	SHDposPacketCount
};

enum BadBlockRank
{
	Rank_Zero = 0,
	Rank_One,
	Rank_Two,
	Rank_Three
};

struct SHDposConfigParams
{
	size_t epochInterval = 6000;         //  sh-dpos a epoch need time ms
	size_t varlitorInterval = 1000;      //  a varlitor to create block time ms     
	size_t blockInterval = 1000;         //  a block created need time ms
	size_t valitorNum = 2;               //  check the varlitor for min num 
	size_t maxValitorNum = 21;           //  check the varlitor for max nu
	size_t verifyVoteNum = 6;            //  the vote chencked about block num 
	bool   isGensisVarNext = false;      
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
		valitorNum = _f.valitorNum;
		maxValitorNum = _f.maxValitorNum;
		verifyVoteNum = _f.verifyVoteNum;
		isGensisVarNext = _f.isGensisVarNext;
	}
	friend std::ostream& operator << (std::ostream& out, SHDposConfigParams const& _f) 
	{
		out << "epochInterval:" << _f.epochInterval << "ms|varlitorInterval:" << _f.varlitorInterval <<
			"ms|blockInterval:" << _f.blockInterval << "ms|checkvarlitorNum:" << _f.valitorNum <<
			"|maxValitorNum:" << _f.maxValitorNum << "|verifyVoteNum:" << _f.verifyVoteNum <<
			"| isGensisVarNext:" << _f.isGensisVarNext;
		return out;
	}
};


struct BadBlockVarlitor
{
	dev::h256       m_headerHash;       // the Block of headerHash
	bytes           m_badBlock;         // the badBlock
	std::set<Address>   m_varlitors;    // the verify field Varlitor
	BadBlockVarlitor()
	{
		m_headerHash = dev::h256();
		m_badBlock = bytes();
		m_varlitors.clear();
	}
	BadBlockVarlitor(dev::h256 const& _hsah, bytes const& _b, Address const& _addr)
	{
		m_headerHash = _hsah;
		m_badBlock = _b;
		m_varlitors.insert(_addr);
	}
};                                                               
struct BadBlocksData
{
	std::map<dev::h256, BadBlockVarlitor> m_BadBlocks;
	BadBlocksData() { m_BadBlocks.clear(); }
    BadBlocksData(dev::h256 const& _hsah, bytes const& _b, Address const& _verifyAddr)
	{
		m_BadBlocks.clear();
		insert(_hsah, _b, _verifyAddr);
	}
    void insert(dev::h256 const& _hsah, bytes const& _b, Address const& _verifyAddr)
	{
		auto ret = m_BadBlocks.find(_hsah);
		if(ret == m_BadBlocks.end())
			m_BadBlocks[_hsah] = { _hsah, _b, _verifyAddr };
		ret->second.m_varlitors.insert(_verifyAddr);
	}
    bool findVerifyVarlitor(dev::h256 const& _hash, Address const& _addr)
	{
		auto ret = m_BadBlocks.find(_hash);
		if(ret == m_BadBlocks.end())
			return false;
		return ret->second.m_varlitors.find(_addr) != ret->second.m_varlitors.end();
	}
    bool findBadBlock(dev::h256 const& _hash)
	{
		return m_BadBlocks.find(_hash) != m_BadBlocks.end();
	}
};

//  Punish the badBlock's varlitor data 
struct PunishBadBlock
{
	BadBlockRank    m_rank; //惩罚等级
	size_t          m_ecoph; //当前轮数
	size_t          m_height; //区块高度
	size_t          m_blockNum; //坏块数量
	bool operator < (PunishBadBlock const& _p) { return m_height < _p.m_height; }
	bool operator == (PunishBadBlock const& _p) { return m_rank == _p.m_rank; }
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
struct SH_DposData :SHDPosStatusMsg
{


};

struct SH_DposBadBlock :SHDPosStatusMsg
{
	dev::SignatureStruct  m_signData;
	bytes               m_badBlock;
	SH_DposBadBlock() :SHDPosStatusMsg()
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

using SHDposMsgQueue = dev::concurrent_queue<SHDposMsgPacket>;

/***************************网络数据包 封装业务数据 start*********************************************/
}
}
