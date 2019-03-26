#pragma once

#include "Common.h"
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

        class Dpos: public SealEngineBase
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
		private:
            bool                CheckValidator(uint64_t _now);                  //验证是否该当前节点出块
            void                kickoutValidator();       //剔除不合格验证人，不能参与下一轮竞选
            void                kickoutcanlidates();      //踢出不合格的候选人, 自定义踢出候选人规则，踢出后也不能成为验证人
            void                countVotes();
            void                disorganizeVotes();
		private:
			std::vector<Address>            m_curr_varlitors;                   //本轮验证人集合
            DposClient const*               m_dpos_cleint;
            int64_t                         m_last_create_block_time;           // 上次进入出块周期时间
            DposConfigParams                m_config;                           // dpos 相关配置

            Logger m_logger{createLogger(VerbosityDebug, "Dpos")};
            Logger m_warnlog{ createLogger(VerbosityWarning, "Dpos") };
        };

        
    }
}