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
        DposVoteState() { m_transations.clear(); }
        ~DposVoteState() { }

    public:
        void            excuteTransation(TransactionBase const& _t, BlockHeader const & _h);
        void            excuteTransation(bytes const& _t, BlockHeader const& _h);
        void            verifyVoteTransation(BlockHeader const& _h, h256s const& _t_hashs);
        void            updateVoteTransation(OnDealTransationResult const& ret);
        void            currReset() { m_onResult.clear(); }
        bool            isVerifyVoteTransation() { return !m_transations.empty(); }
        bool            isSyncVoteTransation() { return !m_onResult.empty(); }
        std::vector<OnDealTransationResult> const& getOnTransationResult() { return m_onResult; }
        std::vector<DposTransaTionResult> const& getTransationResult() { return m_transations; }

    private:
        std::vector<DposTransaTionResult>      m_transations;      //等待验证的交易结果缓存
        std::vector<OnDealTransationResult>    m_onResult;         //处理后的交易结果缓存 等待同步
        Logger                                 m_logger { createLogger(VerbosityDebug, "DposVote") };
    };
}
}