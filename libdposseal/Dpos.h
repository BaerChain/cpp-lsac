#pragma once

#include "Common.h"
#include <libbrccore/SealEngine.h>
#include <libbrccore/Common.h>
#include <libp2p/Common.h>
#include <libdevcore/Worker.h>
#include "DposClient.h"
#include "DposHostCapability.h"

namespace dev
{
    namespace bacd
    {
        using namespace dev ::brc;
        typedef std::function<BlockHeader ()> OnGetHeader;

        class Dpos: public SealEngineBase , Worker
        {
        public:
            Dpos();
            ~Dpos();
            static std::string  name(){ return "Dpos"; }
            static void         init();
            unsigned            ver(){return 1001;};
            unsigned            revision() const override { return 1; }
            unsigned            sealFields() const override { return 2; }
            strings             sealers() const override { return { "cpu" }; };
            void                generateSeal(BlockHeader const& _bi) override;
            void                initConfigAndGenesis(ChainParams const& m_params);
            void                setDposClient(DposClient const* _c) { m_dpos_cleint = _c; }
            DposConfigParams const& dposConfig() { return m_config; }
			std::vector<Address> const& currVarlitors() { return m_curr_varlitors; }
			
            bool                isBolckSeal(uint64_t _now);
            bool                checkDeadline(uint64_t _now);           //验证出块时间周期
			void                tryElect(uint64_t _now);   //判断是否完成了本轮出块，选出新一轮验证人

			inline void         initNet(std::weak_ptr<DposHostcapality> _host) { m_host = _host; }
            inline void         startGeneration() { setName("Dpos"); startWorking(); }   //loop 开启 

		public:
			void                onDposMsg(NodeID _nodeid, unsigned _id, RLP const& _r);
			void                requestStatus(NodeID const & _nodeID, u256 const & _peerCapabilityVersion);

		protected:
			void                workLoop() override; 
		private:
			void                sealAndSend(NodeID const& _nodeid, DposPacketType _type, RLPStream const& _msg_s);   // send msg to nodeId
			void                brocastMsg(DposPacketType _type, RLPStream& _msg_s);       //brocastMsg about SH-Dpos           
		private:
            bool                CheckValidator(uint64_t _now);                  //验证是否该当前节点出块
            void                kickoutValidator();       //剔除不合格验证人，不能参与下一轮竞选
            void                kickoutcanlidates();      //踢出不合格的候选人, 自定义踢出候选人规则，踢出后也不能成为验证人
            void                countVotes();
            void                disorganizeVotes();
		public:
			// verify BadBlock if verfy field will to add bad_msg
			VerifiedBlockRef    verifyBadBlock(Address const& _verfyAddr, bytes const& _block, ImportRequirements::value _ir = ImportRequirements::OutOfOrderChecks);
		private:
			bool                sign_send_badBlock(const Secret &sec, bytes const& _badBlock);
		
		private:
			std::vector<Address>            m_curr_varlitors;                   //本轮验证人集合
            DposClient const*               m_dpos_cleint;
            int64_t                         m_last_create_block_time;           // 上次进入出块周期时间
            DposConfigParams                m_config;                           // dpos 相关配置

			mutable  Mutex                  m_mutex;                            // lock for msg_data
			std::weak_ptr<DposHostcapality> m_host;                             // SH-Dpos network
			DposMsgQueue                    m_msg_queue;                        // msg-data

			BadBlocks                       m_badBlocks;                        // SH-Dpos the varify field blockes
			BadVarlitors                    m_badVarlitor;                      //  SH-Dpos the varify field blockes for varlitors

            Logger m_logger{createLogger(VerbosityDebug, "Dpos")};
            Logger m_warnlog{ createLogger(VerbosityWarning, "Dpos") };
        };

        
    }
}