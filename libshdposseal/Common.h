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
// bad block num
enum BadBlockNum
{
    BadNum_zero = 0,
    BadNum_One = 3,
    BadNum_Two = 7,
    BadNum_Three = 12
};

//惩罚出块轮数或者块数
enum PunishBlcokNum
{
    BlockNum_Zero = 0,
    BlockNum_One = 1,    //剔除1轮出块资格
    BlockNum_Two = 100,  //剔除100块出块资格
    BlockNum_Max
};

enum CreaterType
{
	Varlitor = 0,
	Canlitor
};

struct BadBlockNumPunish
{
    std::map<size_t, size_t> m_badBlockNumPunish;
    BadBlockNumPunish()
    {
        m_badBlockNumPunish.insert(std::make_pair<size_t, size_t>(BadNum_zero, BlockNum_Zero));
        m_badBlockNumPunish.insert(std::make_pair<size_t, size_t>(BadNum_One, BlockNum_One));
        m_badBlockNumPunish.insert(std::make_pair<size_t, size_t>(BadNum_Two, BlockNum_Two));
        m_badBlockNumPunish.insert(std::make_pair<size_t, size_t>(BadNum_Three, BlockNum_Max));
    }
};

// 惩罚信息
struct BadBlockPunish
{
    size_t m_badBlockNum;   //坏块数量
    size_t m_punishEcoph;   // 需要惩罚的不出块轮数或块数
    size_t m_currentEcoph;  //当前惩罚不出块轮数或者块数
    bool m_isPunish;        //是否惩罚 true表示未惩罚完成
    BadBlockPunish(size_t _badBlockNum = 0, size_t _punishEcoph = 0, size_t _crrentEcoph = 0,
        bool _isPunish = false)
      : m_badBlockNum(_badBlockNum),
        m_punishEcoph(_punishEcoph),
        m_currentEcoph(_crrentEcoph),
        m_isPunish(_isPunish)
    {}
    void streamRLP(RLPStream& _s) const
	{
		_s << m_badBlockNum << m_punishEcoph << m_currentEcoph << (size_t)m_isPunish;
	}
    void populate(RLP const& _l)
	{
		int index = 0;
		try
		{
			m_badBlockNum = _l[index = 0].toInt<size_t>();
			m_punishEcoph = _l[index = 1].toInt<size_t>();
			m_currentEcoph = _l[index = 2].toInt<size_t>();
			m_isPunish = (bool)_l[index = 3].toInt<size_t>();
		}           
		catch(Exception& ex)
		{
			cerror << " error populate BadBlockPunish index:" << index;
			throw;
		}
	}
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
    bytes streamRLPByte() const
	{
		RLPStream _s;
		_s.appendList(3);
		_s << (u256)m_headerHash;
		_s << m_badBlock;
		_s.append<Address>(m_varlitors);

		return _s.out();
	}
    void populate(RLP const& _r)
	{
	    try
		{
			int index = 0;
			m_headerHash = (dev::h256)_r[index = 0].toInt<u256>();
			m_badBlock = _r[index = 1].toBytes();
			m_varlitors = _r[index = 2].toSet<Address>();
		}
        catch(Exception& ex)
		{
			cerror << " populate BadBlockVarlitor error index:";
			throw;
		}
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

    void streamRLP(RLPStream& _s) const
	{
		size_t num = m_BadBlocks.size() +1;
		_s.appendList(num);
		_s << m_BadBlocks.size();
        for( auto val : m_BadBlocks)
		{
			_s.append(std::make_pair(val.first, val.second.streamRLPByte()));
		}
	}
    void populate(RLP const& _r)
	{
		try
		{
			size_t num = _r[0].toInt<size_t>();
            for (int i=1; i< num; i++)
            {
				auto _p = _r[i].toPair<dev::h256, bytes>();
				BadBlockVarlitor _bv;
				_bv.populate(RLP(_p.second));
				m_BadBlocks[_p.first] = _bv;
            }
		}
		catch(Exception& ex)
		{

		}
	}
};

//  Punish the badBlock's varlitor data 
//struct PunishBadBlock
//{
//	BadBlockRank    m_rank; //惩罚等级
//	size_t          m_ecoph; //当前轮数
//	size_t          m_height; //区块高度
//	size_t          m_blockNum; //坏块数量
//	bool operator < (PunishBadBlock const& _p) { return m_height < _p.m_height; }
//	bool operator == (PunishBadBlock const& _p) { return m_rank == _p.m_rank; }
//};

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
