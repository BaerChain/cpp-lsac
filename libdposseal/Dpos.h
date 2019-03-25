#pragma once

#include "Common.h"
#include "DposHostCapability.h"
#include <libethcore/SealEngine.h>
#include <libethcore/Common.h>
#include <libp2p/Common.h>
#include <libdevcore/Worker.h>
#include "DposClient.h"

namespace dev
{
    namespace bacd
    {
        using namespace dev ::eth;
        typedef std::function<BlockHeader ()> OnGetHeader;

        class Dpos: public SealEngineBase , Worker
        {
        public:
            Dpos();
            ~Dpos();
            static std::string  name(){ return "Dpos"; }
            static void         init();
            unsigned            revision() const override { return 1; }
            unsigned            sealFields() const override { return 2; }
            strings             sealers() const override { return { "cpu" }; };
            void                generateSeal(BlockHeader const& _bi) override;
            void                initConfigAndGenesis(ChainParams const& m_params);
            void                setDposClient(DposClient const* _c) { m_dpos_cleint = _c; }
            void                setHeaderDposData(BlockHeader& _h)  { _h.setDposContext(m_dpos_context); }
            DposContext const&  dposContext() { return m_dpos_context; }  
            DposConfigParams const& dposConfig() { return m_config; }

            void                initEnv(std::weak_ptr<DposHostcapality> _host);
            inline void         startGeneration() { setName("Dpos"); startWorking(); }   //loop 开启
        protected:
            void                workLoop() override;
        public:
            void                requestStatus(NodeID const & _nodeID, u256 const & _peerCapabilityVersion);
            void                onDposMsg(NodeID _nodeid, unsigned _id, RLP const& _r);
        private:
            void                sealAndSend(NodeID const& _nodeid, DposPacketType _type, RLPStream const& _msg_s);
            void                brocastMsg(DposPacketType _type, RLPStream& _msg_s);   // 广播消息
        public:
            bool                isBolckSeal(uint64_t _now);
            bool                checkDeadline(uint64_t _now);           //验证出块时间周期
            void                tryElect(uint64_t _now);   //判断是否完成了本轮出块，选出新一轮验证人
            void                verifyTransationVote(BlockHeader const& _bi);
            void                varlitorAddBlockNum(Address const& _add, size_t _num = 1) { m_dpos_context.varlitorAddBlockNum(_add, _num); }
            void                printDposData(DposContext const& _d);
        private:
            void                initVarlitorByGenesis();
            void                initDataByCurrBlock();    //根据当前块 初始化dpos数据
            bool                CheckValidator(uint64_t _now);                  //验证是否该当前节点出块
            void                kickoutValidator();       //剔除不合格验证人，不能参与下一轮竞选
            void                kickoutcanlidates();      //踢出不合格的候选人, 自定义踢出候选人规则，踢出后也不能成为验证人
            void                countVotes();
            void                disorganizeVotes();
            void                owenSortDposValitor(std::vector<DposVarlitorVote>& _varlitors);
        private:
            void                updateVoteData(OnDealTransationResult const& _ret);
            void                dealVoteDatas();
            void                dealVoteData(OnDealTransationResult const& _ret);
            void                syncVoteData();
            bool                isVarlitor(Address const& _add);
        private:
            bool                able_to_canlidate(Address const& _addr);        //能否成为候选人
        public:                 
            void                giveBackBlanceToVote(Address const& /*_voter*/, EDposDataType /*_type*/)    {} //投票人 撤回投票 或者投票失效，归还抵押代币

        private:
            mutable  Mutex                  m_mutex;
            std::weak_ptr<DposHostcapality> m_host;
            DposMsgQueue                    m_msg_queue;

            DposContext                     m_dpos_context;                     //本地数据 每轮出块开始通过parentHeader初始化
            std::vector<Address>            m_genesis_varlitor;                 //创世验证集合
            DposClient const*               m_dpos_cleint;
            int64_t                         m_last_dposDataMsg_time;
            int64_t                         m_last_create_block_time;           // 上次进入出块周期时间
            DposConfigParams                m_config;                           // dpos 相关配置

            Logger m_logger{createLogger(VerbosityDebug, "Dpos")};
            Logger m_warnlog{ createLogger(VerbosityWarning, "Dpos") };
        };

        
    }
}