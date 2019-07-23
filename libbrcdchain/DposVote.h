#pragma once
#include "State.h"
#include <libdevcore/Common.h>
#include <libdevcrypto/Common.h>
#include <libbvm/ExtVMFace.h>
#include <libbrccore/config.h>

#define VOTINGDIVIDENDCYCLE 1562898600000
//#define VOTINGTIME 2 * 60 * 1000
#define CALCULATEDINCOMETIME 1562898900000
#define RECEIVINGINCOMETIME 1562899200000
#define SECONDVOTINGDIVIDENDCYCLE 1562899500000

namespace dev
{
namespace brc
{
const Address SysVarlitorAddress { "000000000067656e657369735661726c69746f72" };
const Address SysCanlitorAddress { "0000000067656e6573697343616e646964617465" };

//投票类型
enum VoteType
{
    ENull = 0,
    EBuyVote,
    ESellVote,
    ELoginCandidate,
    ELogoutCandidate,
    EDelegate,
    EUnDelegate
};



struct DposVarlitorVote
{
	Address     m_addr;
	size_t      m_vote_num;
	size_t      m_block_num;
	DposVarlitorVote(DposVarlitorVote const& _d): m_addr(_d.m_addr), m_vote_num(_d.m_vote_num),m_block_num(_d.m_block_num)
	{}
	DposVarlitorVote(Address const& _addr, size_t _num, size_t _block_num):m_addr(_addr), m_vote_num(_num), m_block_num(_block_num)
	{}
};
inline bool dposVarlitorComp(DposVarlitorVote const& _d1, DposVarlitorVote const& _d2)
{
    //
    if(_d1.m_vote_num > _d2.m_vote_num)
        return true;
    else if(_d1.m_vote_num < _d2.m_vote_num)
        return false;
    else
        return _d1.m_block_num > _d2.m_block_num;
}

class DposVote
{
public:
    DposVote(State& _state): m_state(_state) {}
    ~DposVote(){}
    void setState(State& _s) { m_state = _s; }
public:
	void verifyVote(Address const& _from, EnvInfo const& _envinfo, std::vector<std::shared_ptr<transationTool::operation>> const& _ops);
	const std::vector<PollData>  VarlitorsAddress() const { return m_state.vote_data(SysVarlitorAddress); }
	const std::vector<PollData>  CanlitorAddress() const { return m_state.vote_data(SysCanlitorAddress); }
    std::pair <uint32_t, Votingstage> returnVotingstage(EnvInfo const& _envinfo) const;
	void getSortElectors(std::vector<Address>& _electors, size_t _num, std::vector<Address> _ignore) const;

	inline std::vector<PollData>  getElectors() const { return m_state.vote_data(SysElectorAddress); }

private: 
    State&      m_state;
    Logger      m_logger { createLogger(VerbosityDebug, "dposVote") };
};

}
}