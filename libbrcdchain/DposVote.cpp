#include "DposVote.h"
#include <unordered_map>

bool dev::brc::DposVote::verifyVote(Address const & _from, Address const & _to, size_t _type, size_t tickets)
{
    if(_type <= ENull || _type > EUnDelegate)
    {
        LOG(m_logger) << "the vote type is error !";
        return false;
    }
	std::cout << BrcYellow " verifyVote ... " << "_from:" << _from
		<< " _to:" << _to << " _type:" << _type << " _num:" << tickets;
    VoteType dType = (VoteType)_type;
    switch(dType)
    {
        case dev::brc::ENull:
        break;
		case dev::brc::EBuyVote:
		{
			if(m_state.BRC(_to) < tickets* BALLOTPRICE)
				return false;
			return true;
		}
		break;
		case dev::brc::ESellVote:
		{ 
			if(tickets > (size_t)m_state.ballot(_from))
				return false;

			return true;
		}
		break;
        case dev::brc::ELoginCandidate:
        {
            //申请为竞选人
            //初步 验证 是否为本人申请
            if(_from != _to)
                return false;
            std::unordered_map<Address, u256> _vote = m_state.voteDate(SysElectorAddress);
            if(_vote.find(_from) != _vote.end())
                return false;
            //TO DO 添加其他验证条件 

            return true;
        }
        break;
        case dev::brc::ELogoutCandidate:
        {
            //撤销成为竞选人
            if(_from != _to)
                return false;
            std::unordered_map<Address, u256> _vote = m_state.voteDate(SysElectorAddress);
            if(_vote.find(_from) == _vote.end())
                return false;

            return true;
        }
        break;
        case dev::brc::EDelegate:
        {
            //投票
            if(m_state.ballot(_from) < tickets)
            {
                LOG(m_logger) << " Adddress:" << _from << " not have enough tickts:" << tickets;
                return false;
            }
            //验证 竞选人是否存在
            std::unordered_map<Address, u256> _vote = m_state.voteDate(SysElectorAddress);
            if(_vote.find(_to) == _vote.end())
            {
                LOG(m_logger) << "the Elector:" << _to << " don't have !";
                return false;
            }
            return true;
        }
        break;
        case dev::brc::EUnDelegate:
        {
            //撤销投票
            std::unordered_map<Address, u256> _vote = m_state.voteDate(_from);
            auto ret = _vote.find(_to);
            if(ret == _vote.end() || ret->second < tickets)
            {
                LOG(m_logger) << " Address:" << _from << " not voted:" << tickets << " tickets to :" << _to;
                return false;
            }
            return true;
        }
        break;
        default:
        break;
    }
    return false;
}

void dev::brc::DposVote::getSortElectors(std::vector<Address>& _electors, size_t _num) const
{
	const std::unordered_map<Address, u256> _eletors = getElectors();

	std::vector<DposVarlitorVote> _v;
    for(auto val : _eletors)
		_v.push_back({ val.first, (size_t)m_state.poll(val.first), 0 });
	//根据投票数量排序  根据出块数量二级排序
	std::sort(_v.begin(), _v.end(), dposVarlitorComp);

	//在比较属性相同的区段数据打乱顺序
	//取出相同区段数据
	std::vector<std::vector<DposVarlitorVote> > randVarlitors;
	std::vector<DposVarlitorVote> currVarlitors;
	for(auto val : _v)
	{
		if(currVarlitors.empty())
		{
			currVarlitors.push_back(val);
			continue;
		}
		if(currVarlitors[0].m_vote_num == val.m_vote_num && currVarlitors[0].m_block_num == val.m_block_num)
		{
			currVarlitors.push_back(val);
		}
		else
		{
			std::vector<DposVarlitorVote> tempVar;
			tempVar.assign(currVarlitors.begin(), currVarlitors.end());
			randVarlitors.push_back(tempVar);
			currVarlitors.clear();
			currVarlitors.push_back(val);
		}
	}
	if(!currVarlitors.empty())
		randVarlitors.push_back(currVarlitors);
	//区段随机 
	_electors.clear();
	size_t get_num = 0;
	for(auto val : randVarlitors)
	{
		if(val.size() > 1)
		{
			//随机顺序
			int range = val.size();
			srand(utcTimeMilliSec());
			for(int i = 0; i < range; i++)
			{
				int j = rand() % range;
				DposVarlitorVote temp_var = val[i];
				val[i] = val[j];
				val[j] = temp_var;
			}
		}
		for(auto var : val)
		{
			_electors.emplace_back(Address(var.m_addr));
			if(++get_num >= _num)
				return;
		}
	}
}

void dev::brc::DposVote::voteLoginCandidate(Address const& _addr)
{
	m_state.addSysVoteDate(SysElectorAddress, _addr);
}

void dev::brc::DposVote::voteLogoutCandidate(Address const& _addr)
{
	m_state.subSysVoteDate(SysElectorAddress, _addr);
}
