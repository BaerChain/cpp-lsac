#pragma once

#include "Common.h"
#include "DposHostCapability.h"
#include <libethcore/SealEngine.h>
#include <libethcore/Common.h>
#include <libp2p/Common.h>
#include <libdevcore/Worker.h>

namespace dev
{
    namespace bacd
	{
	using namespace dev ::eth;
	    class Dpos: public SealEngineBase, Worker
		{
		public:
			Dpos();
			~Dpos() { stopWorking(); }
			static std::string  name(){ return "Dpos"; }
			unsigned            revision() const override { return 1; }
			unsigned            sealFields() const override { return 2; }
			strings             sealers() const override { return { "cpu" }; };
			void                generateSeal(BlockHeader const& _bi) override;
			static void         init();
			void                initEnv(std::weak_ptr<DposHostcapality> _host);
			inline void         startGeneration()   { setName("Poa"); startWorking(); }   //loop 开启
			inline void         SetCurrValitor(Address const& _addr) { m_curr_valitor = Address(_addr); }
		protected:
			void                workLoop() override;
		public:
			void                onPoaMsg(NodeID _nodeid, unsigned _id, RLP const& _r);
			void                requestStatus(NodeID const& _nodeID, u256 const& _peerCapabilityVersion);
		private:
			// 广播消息
			void                brocastMsg(DposPacketType _type, RLPStream& _msg_s);
			//指定发送
			void                sealAndSend(NodeID const& _nodeid, DposPacketType _type, RLPStream const& _msg_s);

		public:
			void                updateDposData(EDposDataType _type, Address const& _addr_source, Address const& _addr_target);

			bool                isBolckSeal(uint64_t _last_time ,uint64_t _now);
			bool                checkDeadline(uint64_t _last_block_time,uint64_t _now);           //验证出块时间周期
		private:
			bool                CheckValidator(uint64_t _now);                  //验证是否该当前节点出块
			bool                tryElect(uint64_t _last_time, uint64_t _now);   //判断是否完成了本轮出块，选出新一轮验证人
			void                kickoutValidator(unsigned int _prveslot);       //剔除不合格验证人，不能参与下一轮竞选
			void                countVotes(){}
            void                disorganizeVotes(){}
		private:
			mutable  Mutex                  m_mutex;
			std::weak_ptr<DposHostcapality> m_host;
			DposMsgQueue                    m_msg_queue;

			SVarlitor_Voters                m_varlitor_vote;         //验证人对应投票人 1：n
			Voter_Varlitor                  m_vote_varlitor;         //投票人对应验证人 1：1
			Canlidate                       m_canditate;             //候选人

			CurrValirots                    m_curr_valitors;         //本轮验证人集合
			Address                         m_curr_valitor;          //本节点验证人

            //区分创世周期
			unsigned int                    m_prveslot;                //上一轮周期
			unsigned int                    m_currslot;                //本轮周期

		};

        
	}
}