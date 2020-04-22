#pragma once

#include "Common.h"
#include <libbrccore/SealEngine.h>
#include <libbrccore/Common.h>
#include <libp2p/Common.h>
#include <libdevcore/Worker.h>
#include "SHDposClient.h"
#include "SHDposHostCapability.h"
#include <libdevcore/LevelDB.h>
#include <libdevcore/DBFactory.h>
#include <libdevcore/dbfwd.h>
#include <libdevcore/db.h>
#include <libdevcore/Address.h>
#include <boost/filesystem/path.hpp>

namespace dev
{
    namespace bacd
    {
        using namespace dev::brc;
        //const uint64_t timesc_30y =1000 * 60 * 60 * 24 * 365 * 30;
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
            void                populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const override;
            virtual  void       verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent = BlockHeader(), bytesConstRef _block = bytesConstRef()) const override;
            void                initConfigAndGenesis(ChainParams const& m_params);
            void                setDposClient(SHDposClient const* _c) { m_dpos_cleint = _c; }
            SHDposConfigParams const& dposConfig() { return m_config; }

            bool                isBolckSeal(uint64_t _now);
            bool                checkDeadline(uint64_t _now);           //check the create block time_cycle
            inline void         startGeneration() { setName("SHDpos"); startWorking(); }   //loop 开启

        protected:
            void                workLoop() override;

        private:
            /// Verify whether the current_node blocks
            bool                CheckValidator(uint64_t _now);

            /// If the super_node offline , verify standby_node to create_block
            ///@param super_addr: offline's node
            ///@param own_addr: It could be all of the create_nodes
            bool                verify_standby(int64_t block_time, Address const& own_addr) const;

        private:
            std::vector<Address>            m_curr_varlitors;                   //本轮验证人集合
            SHDposClient const*             m_dpos_cleint;
            int64_t                         m_next_block_time;                  // next 进入出块周期时间
            int64_t                         m_last_block_time;
            SHDposConfigParams              m_config;                           // dpos 相关配置
            Logger m_logger{createLogger(VerbosityDebug, "SH-Dpos")};
            Logger m_warnlog{ createLogger(VerbosityWarning, "SH-Dpos") };
        };


    }
}
