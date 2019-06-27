#pragma once
#include "State.h"
#include <libdevcore/Common.h>
#include <libdevcrypto/Common.h>

namespace dev
{
namespace brc
{

//using SysElectorAddress = ElectorAddress;    // 竞选人集合地址
//using SysVarlitorAddress = VarlitorAddress;  // 验证人集合地址

const Address SysElectorAddress  { "0x000000000000456c6563746f7241646472657373" };
const Address SysVarlitorAddress { "000000000067656e657369735661726c69746f72" };
const Address SysCanlitorAddress { "0000000067656e6573697343616e646964617465" };
const Address SystemVoteBrcAddress {"000000766f746542726353797374656d41646472"};

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
    void verifyVote(Address const& _from, Address const& _to, size_t _type, u256 tickets = 0);
	void verifyVote(Address const& _from, std::vector<std::shared_ptr<transationTool::operation>> const& _ops);
	std::map<Address, u256>  VarlitorsAddress() const { return m_state.voteDate(SysVarlitorAddress); }
	std::map<Address, u256>  CanlitorAddress() const { return m_state.voteDate(SysCanlitorAddress); }
	void getSortElectors(std::vector<Address>& _electors, size_t _num, std::vector<Address> _ignore) const;	
    void addVote(Address const& _id, Address const& _recivedAddr, u256 _value) { m_state.addVote(_id, _recivedAddr, _value);} 
    void subVote(Address const& _id, Address const& _recivedAddr, u256 _value) { m_state.subVote(_id, _recivedAddr, _value);} 
    void voteLoginCandidate(Address const& _addr);    
    void voteLogoutCandidate(Address const& _addr); 

	Secret  getVarlitorSecret(Address const& /*_add*/) { return Secret(); }


public:
    std::map<Address, u256> getVoteDate(Address const& _id)const { return m_state.voteDate(_id);}
	inline std::map<Address, u256>  getElectors() const { return m_state.voteDate(SysElectorAddress); }
    
private: 
    State&      m_state;
    Logger      m_logger { createLogger(VerbosityDebug, "dposVote") };
};

}
}