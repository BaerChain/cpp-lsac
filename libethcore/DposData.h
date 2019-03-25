#pragma once
#include "Common.h"
#include <iostream>
#include <libdevcore/RLP.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Exceptions.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/Address.h>

namespace dev
{
    namespace eth
    {
    struct SVarlitor_Voters
    {
        std::map<Address, std::set<Address> > m_varlitor_voter;  //1：n
        SVarlitor_Voters() { m_varlitor_voter.clear(); }
        void operator = (SVarlitor_Voters const& _s)
		{
			m_varlitor_voter.clear();
		    for( auto val : _s.m_varlitor_voter)
			{
				std::set<Address> _adds;
				_adds.clear();
				_adds.insert(val.second.begin(), val.second.end());
				m_varlitor_voter.insert(std::make_pair(val.first, _adds));
			}
		}
        inline void clear() { m_varlitor_voter.clear(); }
        unsigned int voterCount(const Address& _addr_varlitor) const
		{
			auto ret = m_varlitor_voter.find(_addr_varlitor);
			if(ret == m_varlitor_voter.end())
				return 0;
			return ret->second.size();
		}
        //会覆盖之前存在的头片信息
        inline void insert(const Address& _addr_varlitor, const Address& _addr_voter)
		{
			auto ret = m_varlitor_voter.find(_addr_varlitor);
			if(ret == m_varlitor_voter.end())
			{
				std::set<Address> voters;
				voters.insert(_addr_voter);
				m_varlitor_voter[_addr_varlitor] = voters;
			}
			else
			{
				ret->second.insert(_addr_voter);
			}
		}
        inline void del_old_varlitor(Address const& _addr_varlitor, Address const& _addr_vote)
		{
			auto ret = m_varlitor_voter.find(_addr_varlitor);
			if(ret == m_varlitor_voter.end())
				return;
			auto ret_vote = ret->second.find(_addr_vote);
			if(ret_vote == ret->second.end())
				return;
			ret->second.erase(ret_vote);
			if(ret->second.size() <= 0)
				m_varlitor_voter.erase(ret);
		}
        inline void add_vartor_vote(Address const& _addr_varlitor, Address const& _addr_vote)
		{
			auto ret = m_varlitor_voter.find(_addr_varlitor);
			if(ret == m_varlitor_voter.end())
			{
				std::set<Address> vote_set;
				vote_set.insert(_addr_vote);
				m_varlitor_voter.insert(std::make_pair(_addr_varlitor, vote_set));
				return;
			}
			ret->second.insert(_addr_vote);
		}
    };

    struct DposContext
    {
        std::set<Address>           canlidates;                 //  候选人集合
        SVarlitor_Voters            varlitors_votes;            //  验证人对应投票人 1：n
        std::map<Address, Address>  vote_varlitor;              //  投票人对应验证人 1：1
        std::vector<Address>        curr_varlitor;              //  当前轮对应验证人 顺序固定
        std::map<Address, size_t>   varlitor_block_num;         //  验证人 本轮出块数
        DposContext()
		{
			canlidates.clear();
			varlitors_votes.clear();
			vote_varlitor.clear();
			curr_varlitor.clear();
			varlitor_block_num.clear();
		}
        DposContext& operator = (const DposContext& _d)
		{
			clear();
			canlidates.insert(_d.canlidates.begin(), _d.canlidates.end());
			varlitors_votes = _d.varlitors_votes;
			//varlitors_votes.m_varlitor_voter.insert(_d.varlitors_votes.m_varlitor_voter.begin(), _d.varlitors_votes.m_varlitor_voter.end());
			vote_varlitor.insert(_d.vote_varlitor.begin(), _d.vote_varlitor.end());
			curr_varlitor.assign(_d.curr_varlitor.begin(), _d.curr_varlitor.end());
			varlitor_block_num.insert(_d.varlitor_block_num.begin(), _d.varlitor_block_num.end());
			return *this;
		}
        void clear()
		{
			canlidates.clear();
			varlitors_votes.clear();
			vote_varlitor.clear();
			curr_varlitor.clear();
			varlitor_block_num.clear();
		}
        void streamRLPFields(RLPStream& _s) const
		{
			//cdebug << "!start streamRLPFields dposcontext ....";
			RLPStream s;
			s.appendList(5);
			//封装候选人
			s.append<Address>(canlidates);
			//cdebug << "streamRLP: canlidates:" << canlidates.size();

			//封装 当前轮验证人
			s.appendVector<Address>(curr_varlitor);
			//cdebug << "streamRLP: curr_varlitor:" << curr_varlitor.size();


			//封装 投票人对应验证人 1:1
			RLPStream _s_vote;
			unsigned int vote_size = vote_varlitor.size();
			_s_vote.appendList(vote_size + 1);
			_s_vote << vote_size;
			for(auto val : vote_varlitor)
			{
				_s_vote << std::make_pair(val.first, val.second);
			}
			s.append(_s_vote.out());
			//cdebug << "streamRLP: vote_varlitor:" << _s_vote.out().size();

			//封装  验证人对应投票人 1：n
			RLPStream _s_var;
			unsigned int var_size = varlitors_votes.m_varlitor_voter.size();
			_s_var.appendList(var_size + 1);
			_s_var << var_size;
			//cdebug << EthRed"streamRLP var_size：" << var_size;
			for(auto val : varlitors_votes.m_varlitor_voter)
			{
				RLPStream _s_var_votes;
				_s_var_votes.appendList(1);
				_s_var_votes.append<Address>(val.second);
				//cdebug << EthRed"streamRLP m_varlitor_voter voters size：" << val.second.size()<<"|"<< _s_var_votes.out().size();
				_s_var << make_pair(val.first, _s_var_votes.out());
			}
			//cdebug << "streamRLP: m_varlitor_voter:" << _s_var.out().size();
			s.append(_s_var.out());

			//封装 验证人 本轮出块数
			RLPStream _s_block_num;
			size_t _var_block_size = varlitor_block_num.size();
			_s_block_num.appendList(_var_block_size + 1);
			_s_block_num << _var_block_size;
			for(auto val : varlitor_block_num)
			{
				_s_block_num << std::make_pair(val.first, val.second);
			}
			s.append(_s_block_num.out());

			//cdebug << "streamRLP: dposData:" << s.out().size();
			_s.append(s.out());
			//cdebug << "!!end streamRLPFields dposcontext ....";
		}
        void populate(RLP const& _header, int const& field)
		{
			//cdebug << "!!start populate dposcontext ....";
			int _f = 0;
			RLP _r;
			try
			{
				const bytes _b = _header[field].toBytes();
				//cdebug << "populate: dposData size:" << _b.size();
				_r = RLP(_b);

				//解析候选人
				canlidates = _r[_f = 0].toSet<Address>();
				//cdebug << "populate: canlidates:" << canlidates.size();

				//解析 当前轮验证人
				curr_varlitor = _r[_f = 1].toVector<Address>();
				//cdebug << "populate: curr_varlitor:" << curr_varlitor.size();

				//解析 投票人对应验证人 1:1
				const bytes _b_vote = _r[_f = 2].toBytes();
				RLP _r_vote = RLP(_b_vote);
				unsigned int var_size = _r_vote[0].toInt<unsigned int>();
				for(unsigned int i = 1; i <= var_size; i++)
				{
					std::pair<Address, Address> _vote_pair = _r_vote[i].toPair<Address, Address>();
					vote_varlitor.insert(_vote_pair);
				}
				//cdebug << "populate: vote_varlitor:" << vote_varlitor.size();
				//解析 验证人对应投票人 1:n
				const bytes _b_var = _r[_f = 3].toBytes();
				RLP _r_var = RLP(_b_var);
				unsigned int vote_size = _r_var[0].toInt<unsigned int>();
				//cdebug << EthRed"populate var_size：" << vote_size;
				for(unsigned int i = 1; i <= vote_size; i++)
				{
					std::pair<Address, bytes> _var_pair = _r_var[i].toPair<Address, bytes>();
					RLP _r_votes = RLP(_var_pair.second);
					std::set<Address> votes = _r_votes[0].toSet<Address>();
					//cdebug << EthRed"populate m_varlitor_voter voters：" << votes.size()<<"|"<<_var_pair.second.size();
					varlitors_votes.m_varlitor_voter.insert(std::make_pair(_var_pair.first, votes));
				}

				//解析 验证人出块数量
				const bytes _b_var_num = _r[_f = 4].toBytes();
				RLP _r_var_num = RLP(_b_var_num);
				size_t var_block_num = _r_var_num[0].toInt<size_t>();
				for(size_t i = 1; i <= var_block_num; i++)
				{
					std::pair<Address, size_t> _var_num_pair = _r_var_num[i].toPair<Address, size_t>();
					varlitor_block_num[_var_num_pair.first] = _var_num_pair.second;
				}
				//cdebug << "populate: m_varlitor_voter:" << varlitors_votes.m_varlitor_voter.size();  
			}
			catch(Exception const& /*_e*/)
			{
				cwarn << "dposcontext data populate error!";
			}
			//cdebug << "!!end streamRLPFields dposcontext ....";
		}
        //add 出块数量
        void varlitorAddBlockNum(Address const& _add, size_t _num = 1)
		{
			if(std::find(curr_varlitor.begin(), curr_varlitor.end(), _add) == curr_varlitor.end())
			{
				return;
			}
			size_t num = _num;
			auto ret = varlitor_block_num.find(_add);
			if(ret != varlitor_block_num.end())
			{
				num += ret->second;
			}
			varlitor_block_num[_add] = num;
		}
        void printData () const
		{
			DposContext const& _d = *this;
			std::cout << EthYellow" DposData:" << EthYellow;
			std::cout << EthBlue "canlitor:{";
			for(auto val : _d.canlidates)
				std::cout << val << "| ";
			std::cout << "}" << std::endl << EthBlue;

			std::cout << EthBlue "varlitors_votes:{";
			for(auto val : _d.varlitors_votes.m_varlitor_voter)
			{
				std::cout << "[varlitor:" << val.first << "(";
				for(auto item : val.second)
					std::cout<< item << "| ";
				std::cout << ")] ";
			}
			std::cout << "}" << std::endl << EthBlue;

			std::cout << EthBlue "vote_varlitor:{";
			for(auto val : _d.vote_varlitor)
			{
				std::cout << "[vote:" << val.first << "| varlitor:" << val.second<<"] ";
			}
			std::cout << "}" << std::endl << EthYellow;

			std::cout << EthYellow "curr_varlitor:{";
			for(auto val : _d.curr_varlitor)
			{
				std::cout << val << "| ";
			}
			std::cout << "}" << std::endl << EthYellow;

			std::cout << EthYellow " varlitor_block_num: {";
			for(auto val : _d.varlitor_block_num)
			{
				std::cout << "[varlitor:" << val.first << "| num:" << val.second << "] ";
			}
			std::cout << "}" << std::endl << EthYellow;
		}
        friend std::ostream& operator << (std::ostream& out , DposContext& _d)
		{
			out << "canlitor:{";
			for(auto val : _d.canlidates)
				out<< val << "| ";
			out << "}" << std::endl;

			out << "varlitors_votes:{";
			for(auto val : _d.varlitors_votes.m_varlitor_voter)
			{
				out << " [varlitor:" << val.first << " (";
				for(auto item : val.second)
					out<< item << "| ";
				out << ") ]";
			}
			out << "}" << std::endl;

			std::cout << "vote_varlitor:{";
			for(auto val : _d.vote_varlitor)
			{
				out << "[vote:" << val.first << "| varlitor:" << val.second<<"] ";
			}
			out << "}" << std::endl ;

			out <<"curr_varlitor:{";
			for(auto val : _d.curr_varlitor)
			{
				out << val <<"| ";
			}
			out << "}" << std::endl;

			out <<" varlitor_block_num:{";
			for(auto val : _d.varlitor_block_num)
			{
				out << " [varlitor:" << val.first << "| num:" << val.second <<"]";
			}
			out << "}" << std::endl;
			return out;
		}
    };

    }
}