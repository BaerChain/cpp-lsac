#pragma once
#include <iostream>
#include <libdevcore/RLP.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Exceptions.h>
#include <libdevcore/FixedHash.h>
#include <libethcore/TransactionBase.h>
#include <libethcore/BlockHeader.h>
#include "Common.h"

namespace dev
{
namespace bacd
{
    using namespace eth;

    class DposVoteState
    {
    public:
        DposVoteState(DposConfigParams const& _config) { m_transations.clear(); m_config = _config; }
        ~DposVoteState() { }

    public:
        void            excuteTransation(TransactionBase const& _t, BlockHeader const & _h);
        void            excuteTransation(bytes const& _t, BlockHeader const& _h);
        void            verifyVoteTransation(BlockHeader const& _h, h256s const& _t_hashs);
        void            updateVoteTransation(OnDealTransationResult const& ret);
        void            currReset() { m_onResult.clear(); }
        bool            isVerifyVoteTransation() { return !m_transations.empty(); }
        bool            isSyncVoteTransation() { return !m_onResult.empty(); }
        std::vector<OnDealTransationResult>& getOnTransationResult() { return m_onResult; }
        std::vector<DposTransaTionResult> const& getTransationResult() { return m_transations; }
		void            setUpdateTime(int64_t _time) { m_updateTime = _time; }
		int64_t         getUpdateTime() { return m_updateTime; }
    private:            
        bool            insertDposTransaTionResult(DposTransaTionResult const& _d);

    private:
        std::vector<DposTransaTionResult>      m_transations;      //等待验证的交易结果缓存
		int64_t                                m_updateTime;
        std::vector<OnDealTransationResult>    m_onResult;         //验证后的交易结果缓存 等待处理
        DposConfigParams                       m_config;
        Logger                                 m_logger { createLogger(VerbosityDebug, "DposVote") };
    };
}
}