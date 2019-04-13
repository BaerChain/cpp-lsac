#pragma once

#include "Common.h"
#include <libbrccore/SealEngine.h>
#include <libbrccore/Common.h>
#include <libp2p/Common.h>
#include <libdevcore/Worker.h>
#include "SHDposClient.h"
#include "SHDposHostCapability.h"

namespace dev
{
    namespace bacd
    {
        using namespace dev ::brc;
        typedef std::function<BlockHeader ()> OnGetHeader;

        class SHDpos: public SealEngineBase , Worker
        {
        public:
            SHDpos();
            ~SHDpos();
            static std::string  name(){ return "SHDpos"; }
            static void         init();
            unsigned            ver(){return 1001;};
            unsigned            revision() const override { return 1; }
            unsigned            sealFields() const override { return 2; }
            strings             sealers() const override { return { "cpu" }; };
            void                generateSeal(BlockHeader const& _bi) override;
            void                initConfigAndGenesis(ChainParams const& m_params);
            void                setDposClient(SHDposClient const* _c) { m_dpos_cleint = _c; }
            SHDposConfigParams const& dposConfig() { return m_config; }
			std::vector<Address> const& currVarlitors() { return m_curr_varlitors; }
			
            bool                isBolckSeal(uint64_t _now);
            bool                checkDeadline(uint64_t _now);           //验证出块时间周期
			void                tryElect(uint64_t _now);   //判断是否完成了本轮出块，选出新一轮验证人

			inline void         initNet(std::weak_ptr<SHDposHostcapality> _host) { m_host = _host; }
            inline void         startGeneration() { setName("SHDpos"); startWorking(); }   //loop 开启 

		public:
			void                onDposMsg(NodeID _nodeid, unsigned _id, RLP const& _r);
			void                requestStatus(NodeID const & _nodeID, u256 const & _peerCapabilityVersion);

		protected:
			void                workLoop() override; 
		private:
			void                sealAndSend(NodeID const& _nodeid, SHDposPacketType _type, RLPStream const& _msg_s);   // send msg to nodeId
			void                brocastMsg(SHDposPacketType _type, RLPStream& _msg_s);       //brocastMsg about SH-Dpos           
		private:
            bool                CheckValidator(uint64_t _now);                  //验证是否该当前节点出块
            void                kickoutValidator();       //剔除不合格验证人，不能参与下一轮竞选
            void                kickoutcanlidates();      //踢出不合格的候选人, 自定义踢出候选人规则，踢出后也不能成为验证人
            void                countVotes();
            void                disorganizeVotes();

		public:
			// verify BadBlock if verfy field will to add bad_msg
			bool                verifyBadBlock(Address const& _verfyAddr, bytes const& _block, ImportRequirements::value _ir = ImportRequirements::OutOfOrderChecks);
			void                getBadVarlitors(std::map<Address, BadBlockRank >& _mapBadRank);
			void                dellImportBadBlock(bytes const& _block);
            void				disposeBadBlockpunishment();
            void				deleteValidatorBlock(std::vector<Address>& _vector);

		private:
			SignatureStruct     signBadBlock(const Secret &sec, bytes const& _badBlock);
			bool                verifySignBadBlock( bytes const& _badBlock, SignatureStruct const& _signData);
			void                addBadBlockLocal(bytes const& _b, Address const& _verifyAddr);
			void                sendBadBlockToNet(bytes const& _block);

		private:
			std::vector<Address>            m_curr_varlitors;                   //本轮验证人集合
            SHDposClient const*               m_dpos_cleint;
            int64_t                         m_last_create_block_time;           // 上次进入出块周期时间
            SHDposConfigParams                m_config;                           // dpos 相关配置

			mutable  Mutex                  m_mutex;                            // lock for msg_data
			std::weak_ptr<SHDposHostcapality> m_host;                             // SH-Dpos network
			SHDposMsgQueue                    m_msg_queue;                        // msg-data

			std::map<Address, BadBlocksData>  m_badVarlitors;                   // SH-Dpos the varify field blockes for varlitor
			std::map<Address, PunishBadBlock> m_punishBadVarlotor;              // 当前论 惩罚记录 新一轮 reset


            Logger m_logger{createLogger(VerbosityDebug, "SH-Dpos")};
            Logger m_warnlog{ createLogger(VerbosityWarning, "SH-Dpos") };
        };

        
    }
}
