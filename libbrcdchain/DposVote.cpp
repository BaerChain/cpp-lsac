#include "DposVote.h"
#include <unordered_map>

using namespace dev::brc;


void dev::brc::DposVote::verifyVote(Address const& _from, EnvInfo const& _envinfo, std::vector<std::shared_ptr<transationTool::operation>> const& _ops){
    std::shared_ptr<transationTool::vote_operation> p_op = nullptr;
    for(auto const& val : _ops){
		p_op = std::dynamic_pointer_cast<transationTool::vote_operation> (val);
		if(!p_op)
			BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment("oparetion type error!"));
		VoteType dType = (VoteType)p_op->m_vote_type;
		if(dType == EBuyVote || dType == ESellVote || dType == EDelegate || dType == EUnDelegate){
			if(p_op->m_vote_numbers <= 0){
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment("tickets must bigger 0! ticket:"));
			}
		}

		// Function temporary shielding
		if (dType == ELoginCandidate || dType == ELogoutCandidate)
            BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment("have no permissions to do thos functions!"));

        std::pair <uint32_t, Votingstage> _pair = returnVotingstage(_envinfo);
        if(_pair.second != Votingstage::VOTE)
        {
            if (dType == dev::brc::EDelegate || dev::brc::EUnDelegate)
                BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment("The time point that is not currently in the voting phase"));
        }

		switch(dType){
		case dev::brc::ENull:
			break;
		case dev::brc::EBuyVote:
		{
			if(m_state.BRC(_from) < p_op->m_vote_numbers * BALLOTPRICE)
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment("Not enough BRC to buy tickets:"));
		}
		break;
		case dev::brc::ESellVote:
		{
			if(m_state.ballot(_from) < p_op->m_vote_numbers)
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment("not have enough tickets to sell... tickes:"));
			if((p_op->m_vote_numbers * BALLOTPRICE) > m_state.BRC(dev::systemAddress))
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment("system not enough BRC to author, can't sell ticks"));
		}
		break;
		case dev::brc::ELoginCandidate:
		{
			//申请为竞选人
			//初步 验证 是否为本人申请
			if(_from != p_op->m_to)
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment(" Accout not is Candidate"));
            const PollData p_data = m_state.poll_data(SysElectorAddress, _from);
            if(p_data.m_addr == _from){
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment("Accout early is Candidate"));
			}
		}
		break;
		case dev::brc::ELogoutCandidate:
		{
			//撤销成为竞选人
			if(_from != p_op->m_to)
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment(" Accout not is Candidate"));

            const PollData p_data = m_state.poll_data(SysElectorAddress, _from);
            if(p_data.m_addr != _from)
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment(" Accout not is Candidate"));
		}
		break;
		case dev::brc::EDelegate:
		{
			//投票
			if(m_state.ballot(_from) < p_op->m_vote_numbers){
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment(" not have enough ticket to vote..."));
			}
			//verify the Elector exist
            const PollData p_data = m_state.poll_data(_from, p_op->m_to);
            if(p_data.m_addr != p_op->m_to){
				std::string _ex_info = "the Elector:" + toString(p_op->m_to) + " not exist !";
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment(_ex_info));
			}
		}
		break;
		case dev::brc::EUnDelegate:
		{
            const PollData p_data = m_state.poll_data(_from, p_op->m_to);
			if(p_data.m_addr != p_op->m_to || p_data.m_poll < p_op->m_vote_numbers) {
                std::string _ex_info = " Address:" + toString(_from) + " not voted:" + toString(p_op->m_vote_numbers) +
                                       " tickets to :" + toString(p_op->m_to);
                BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment(_ex_info));
            }
		}
		break;
		default:
		break;
		}

	}
	// dynamic_pointer_cast<std::shared_ptr<transationTool::vote_operation>>();
}

void dev::brc::DposVote::getSortElectors(std::vector<Address>& _electors, size_t _num, std::vector<Address> _ignore) const
{
//    std::map<Address, u256> _eletors_temp = getElectors();
//    for (auto it : _ignore)
//    {
//        auto ret = _eletors_temp.find(it);
//        if(ret != _eletors_temp.end())
//			_eletors_temp.erase(ret);
//    }
//    std::vector<DposVarlitorVote> _v;
//    for (auto val : _eletors_temp)
//        _v.push_back({val.first, (size_t)m_state.poll(val.first), 0});
//    //根据投票数量排序  根据出块数量二级排序
//    std::sort(_v.begin(), _v.end(), dposVarlitorComp);
//	int index = 0;
//    for(auto val : _v)
//	{
//		if(++index < _num)
//			return;
//		_electors.push_back(val.m_addr);
//	}
}

std::pair<uint32_t, dev::brc::Votingstage > dev::brc::DposVote::returnVotingstage(const dev::brc::EnvInfo &_envinfo) const
{
    return config::getVotingCycle(_envinfo.number());
}