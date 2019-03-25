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
            unsigned            revision() const override { return 1; }
            unsigned            sealFields() const override { return 2; }
            strings             sealers() const override { return { "cpu" }; };
            void                generateSeal(BlockHeader const& _bi) override;

            void                initGenesieVarlitors(ChainParams const& m_params);
            void                setDposClient(DposClient const* _c) { m_dpos_cleint = _c; }
            void                setHeaderDposData(BlockHeader& _h)  { _h.setDposContext(m_dpos_context); }
            DposContext const&  dposContext() { return m_dpos_context; }
            
            static void         init();

            void                initEnv(std::weak_ptr<DposHostcapality> _host);
            inline void         startGeneration() { setName("Dpos"); startWorking(); }   //loop 开启
        protected:
            void                workLoop() override;
        public:
            void                requestStatus(NodeID const & _nodeID, u256 const & _peerCapabilityVersion);
            void                onDposMsg(NodeID _nodeid, unsigned _id, RLP const& _r);
        private:
            void                sealAndSend(NodeID const& _nodeid, DposPacketType _type, RLPStream const& _msg_s);

        public:
            bool                isBolckSeal(uint64_t _now);
            bool                checkDeadline(uint64_t _now);           //验证出块时间周期
            void                tryElect(uint64_t _now);   //判断是否完成了本轮出块，选出新一轮验证人
        private:
            void                initVarlitorByGenesis();
            bool                CheckValidator(uint64_t _now);                  //验证是否该当前节点出块
            void                kickoutValidator();       //剔除不合格验证人，不能参与下一轮竞选
            void                countVotes();
            void                disorganizeVotes();
        private:
            void                updateVoteData(OnDealTransationResult const& _ret);
            void                dealVoteDatas();
            void                dealVoteData(OnDealTransationResult const& _ret);
            void                syncVoteData();
        public:
            void                verifyTransationVote(BlockHeader const& _bi);
            void                varlitorAddBlockNum(Address const& _add, size_t _num = 1) { m_dpos_context.varlitorAddBlockNum(_add, _num); }
            void                printDposData(DposContext const& _d);

        private:
            // 广播消息
            void                brocastMsg(DposPacketType _type, RLPStream& _msg_s);

            void                initDataByCurrBlock();    //根据当前块 初始化dpos数据
            bool                isVarlitor(Address const& _add);

        private:
            mutable  Mutex                  m_mutex;
            std::weak_ptr<DposHostcapality> m_host;
            DposMsgQueue                    m_msg_queue;

            DposContext                     m_dpos_context;                     //本地数据 每轮出块开始通过parentHeader初始化
            std::vector<Address>            m_genesis_varlitor;                 //创世验证集合
            DposClient const*               m_dpos_cleint;
            int64_t                         m_last_dposDataMsg_time;
            int64_t                         m_last_create_block_time;           // 上次进入出块周期时间

            Logger m_logger{createLogger(VerbosityDebug, "Dpos")};
            Logger m_warnlog{ createLogger(VerbosityWarning, "Dpos") };
        };

        
    }
}