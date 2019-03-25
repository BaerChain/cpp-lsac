#pragma once
/*
    dpos 相关数据结构
*/
#include <iostream>
#include <libdevcore/concurrent_queue.h>
#include <libdevcore/RLP.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Exceptions.h>
#include <libp2p/Common.h>
#include <libdevcore/FixedHash.h>

namespace dev
{
	namespace bacd
    {
		using NodeID = p2p::NodeID;
        extern const unsigned c_protocolVersion;

		const unsigned int epochInterval = 3600;        // 一个出块轮询周期 s
		const unsigned int blockInterval = 3;           // 一个块最短出块时间 
		const unsigned int valitorNum = 5;              // 筛选验证人的人数最低值
		const unsigned int maxValitorNum = 21;          // 最大验证人数量

		enum DposPacketType :byte
		{
			DposStatuspacket = 0x23,
            DposDataPacket,
			DposPacketCount
		};
		enum EDposDataType
		{
            e_loginCandidate =0,         // 成为候选人
            e_logoutCandidate,
            e_delegate,            // 推举验证人
            e_unDelegate,

            e_max,
		};

		using Canlidate = std::set<Address>;                    //候选人集合
		using Voters = std::set<Address>;
		using Varlitor_Voter = std::map<Address, Voters>;  //验证人对应的投票人
		using Voter_Varlitor = std::map<Address, Address>; //投票人对应验证人
		using CurrValirots = std::vector<Address>;         //本轮已确定的验证人，顺序固定
		struct SVarlitor_Voters
		{
			Varlitor_Voter m_varlitor_voter;

			SVarlitor_Voters(){ m_varlitor_voter.clear(); }
			inline void clear(){ m_varlitor_voter.clear(); }
			unsigned int voterCount(const Address& _addr_varlitor) const
			{
				auto ret = m_varlitor_voter.find(_addr_varlitor);
				if(ret == m_varlitor_voter.end())
					return 0;
				return ret->second.size();
			}
			//会覆盖之前存在的头片信息
			inline void insert(const Address& _addr_varlitor, const Address& _addr_voter)
			{
				auto ret = m_varlitor_voter.find(_addr_varlitor);
                if(ret == m_varlitor_voter.end())
				{
					Voters voters;
					voters.insert(_addr_voter);
					m_varlitor_voter[_addr_varlitor] = voters;
				}
				else
				{
					ret->second.insert(_addr_voter);
				}
			}

		};

        // 网络数据包 封装业务数据
		struct DposMsgPacket  
		{
			h512        node_id;     // peer id
			unsigned    packet_id;   //msg id
			bytes       data;        //rlp data

			DposMsgPacket() :node_id(h512(0)), packet_id(0) {}
			DposMsgPacket(h512 _id, unsigned _pid, bytesConstRef _data)
				:node_id(_id), packet_id(_pid), data(_data.toBytes()) {}
		};
		using DposMsgQueue = dev::concurrent_queue<DposMsgPacket>;

        struct DPosStatusMsg
		{
			uint64_t        m_lastTime;
		};

        struct DposDataMsg
		{
		    
		};
    }
}