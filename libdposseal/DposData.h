#pragma once
#include "Common.h"
#include <iostream>
#include <libdevcore/RLP.h>
#include <libdevcrypto/Common.h>
#include <libbrccore/Exceptions.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/Address.h>

namespace dev
{
    namespace brc
    {

    enum EVoteType
	{
	    e_var_vote =0,
        e_voter_val,
	};

    struct SVoteData
	{
		std::map<Address, size_t>   m_addr_num;
		SVoteData() { m_addr_num.clear();  }
        void operator = (SVoteData const& _v)
		{
			m_addr_num.insert(_v.m_addr_num.begin(), _v.m_addr_num.end());
		}
        size_t voteCount() const 
		{
			size_t num = 0;
		    for(auto val : m_addr_num)
			{
				num += val.second;
			}
			return num;
		}
        size_t voteCount(Address const& _addr) const
		{
			auto ret = m_addr_num.find(_addr);
			if(ret == m_addr_num.end())
				return 0;
			return ret->second;
		}
        void insert(Address const& _addr)
		{
			auto ret = m_addr_num.find(_addr);
            if(ret == m_addr_num.end())
			{
				m_addr_num[_addr] = 1;
				return;
			}
			ret->second += 1;
		}
        bool delAddrData(Address const& _addr)
		{
			auto ret = m_addr_num.find(_addr);
			if(ret == m_addr_num.end())
				return false;
			if(ret->second <=1)
		    {
			    m_addr_num.erase(ret);
				return true;
			}
			ret->second -= 1;
			return true;
		}
		void streamRLPFields(RLPStream& _s) const
		{
			size_t num = m_addr_num.size() + 1;
			_s.appendList(num);
			_s << m_addr_num.size();
            for ( auto val : m_addr_num)
            {
				_s << val;
            }
		}
		void populate(RLP const& _r)
		{
			m_addr_num.clear();
		    try
		    {
				size_t Addr_size = _r[0].toInt<size_t>();
				for(unsigned int i = 1; i <= Addr_size; i++)
				{
					std::pair<Address, size_t> _pair = _r[i].toPair<Address, size_t>();
					m_addr_num.insert(_pair);
				}
		    }
			catch(Exception const& /*_e*/)
			{
				cwarn << "dposcontext SVoteData populate error!";
			}
		}
		friend std::ostream& operator << (std::ostream& out, SVoteData& _v)
		{
			for(auto val : _v.m_addr_num)
			{
				out << "Address:" << val.first;
				out << " | num:" << val.second;
				out << std::endl;
			}
			return out;
		}
	};

    struct DposContext
    {
		int64_t                         updateTime;
        std::set<Address>               canlidates;                 //  候选人集合
		std::map<Address, SVoteData>    varlitor_votes;             //  验证人对应投票人 1：n,n
		std::map<Address, SVoteData>    vote_varlitors;             //  投票人对应验证人 1：n,n
        DposContext()
		{
			updateTime = 0;
			canlidates.clear();
			varlitor_votes.clear();
			vote_varlitors.clear();
		}
        DposContext& operator = (const DposContext& _d)
		{
			clear();
			updateTime = _d.updateTime;
			canlidates.insert(_d.canlidates.begin(), _d.canlidates.end());
			varlitor_votes.insert(_d.varlitor_votes.begin(), _d.varlitor_votes.end());
			vote_varlitors.insert(_d.vote_varlitors.begin(), _d.vote_varlitors.end());
			return *this;
		}
        void clear()
		{
			updateTime = 0;
			canlidates.clear();
			varlitor_votes.clear();
			vote_varlitors.clear();
		}
        void insertVote(Address const& _addr, Address const& _vote_addr, EVoteType _type)
		{
			std::map<Address, SVoteData>* _p_map =nullptr;
			if(_type == e_var_vote)
				_p_map = &varlitor_votes;
			else if(_type == e_voter_val)
				_p_map = &vote_varlitors;
			if(!_p_map)
				return;
			auto ret = _p_map->find(_addr);
            if(ret == _p_map->end())
			{
				SVoteData data;
				data.insert(_vote_addr);
				_p_map->insert(std::make_pair(_addr, data));
				return;
			}
			ret->second.insert(_vote_addr);
		}
        // 取消 投票逻辑 使用该接口， 成功取消->true ,未发现要取消的数据->false
        bool delOldVote(Address const& _addr, Address const& _vote_addr, EVoteType _type)
		{
			std::map<Address, SVoteData>* _p_map = nullptr;
			if(_type == e_var_vote)
				_p_map = &varlitor_votes;
			else if(_type == e_voter_val)
				_p_map = &vote_varlitors;
			if(!_p_map)
				return false;
			auto ret = _p_map->find(_addr);
			if(ret == _p_map->end())
				return false;
			if(!ret->second.delAddrData(_vote_addr))
				return false;
			if(ret->second.voteCount() <= 0)
				_p_map->erase(ret);
			return true;
		}
        //统计的票数量 
        size_t getCount(Address const& _addr)
		{
			auto ret = varlitor_votes.find(_addr);
			if(ret == varlitor_votes.end())
				return 0;
			return ret->second.voteCount();
		}
        void SetUpdateTime(int64_t _time)    
		{
			updateTime =_time;
		}
        void streamRLPFields(RLPStream& s) const
		{
			//cdebug << "!start streamRLPFields dposcontext ....";
			//RLPStream s;
			s.appendList(4);
			//封装候选人
			s.append<Address>(canlidates);
			//cdebug << "streamRLP: canlidates:" << canlidates.size();

			//封装 投票人对应验证人
			RLPStream _s_vote;
			size_t vote_size = vote_varlitors.size();
			_s_vote.appendList(vote_size + 1);
			_s_vote << vote_size;
            for ( auto val : vote_varlitors)
            {
				RLPStream _r_addr;
				val.second.streamRLPFields(_r_addr);
				_s_vote << std::make_pair(val.first, _r_addr.out());
            }
			s.append(_s_vote.out());
			//cdebug << "streamRLP: vote_varlitor:" << _s_vote.out().size();

			//封装  验证人对应投票人
			RLPStream _s_var;
			_s_var.appendList(varlitor_votes.size() + 1);
			_s_var << varlitor_votes.size();
            for (auto val : varlitor_votes)
            {
				RLPStream _r_addr;
				val.second.streamRLPFields(_r_addr);
				_s_var << std::make_pair(val.first, _r_addr.out());
            }
			//cdebug << "streamRLP: m_varlitor_voter:" << _s_var.out().size();
			s.append(_s_var.out());

			s << updateTime;
			//cdebug << "!!end streamRLPFields dposcontext ....";
		}
        void populate(bytes const& _data)
		{
			//cdebug << "!!start populate dposcontext ....";
			int _f = 0;
			RLP _r;
			try
			{
				//const bytes _b = _header[field].toBytes();
				//cdebug << "populate: dposData size:" << _b.size();
				_r = RLP(_data);

				//解析候选人
				canlidates = _r[_f = 0].toSet<Address>();
				//cdebug << "populate: canlidates:" << canlidates.size();

				//解析 投票人对应验证人 1:1
				const bytes _b_vote = _r[_f = 1].toBytes();
				RLP _r_vote = RLP(_b_vote);
				unsigned int var_size = _r_vote[0].toInt<unsigned int>();
				for(unsigned int i = 1; i <= var_size; i++)
				{
					std::pair<Address, bytes> _vote_pair = _r_vote[i].toPair<Address, bytes>();
					RLP _r_addr = RLP(_vote_pair.second);
					SVoteData data;
					data.populate(_r_addr);
					vote_varlitors[_vote_pair.first] = data;
				}
				//cdebug << "populate: vote_varlitors:" << vote_varlitors.size();
				//解析 验证人对应投票人 1:n
				const bytes _b_var = _r[_f = 2].toBytes();
				RLP _r_var = RLP(_b_var);
				unsigned int vote_size = _r_var[0].toInt<unsigned int>();
				//cdebug << BrcRed"populate var_size：" << vote_size;
				for(unsigned int i = 1; i <= vote_size; i++)
				{
					std::pair<Address, bytes> _var_pair = _r_var[i].toPair<Address, bytes>();
					RLP _r_votes = RLP(_var_pair.second);
					SVoteData data;
					data.populate(_r_votes);
					varlitor_votes[_var_pair.first] = data;
					//cdebug << BrcRed"populate m_varlitor_voter voters：" << votes.size()<<"|"<<_var_pair.second.size();
				} 

				updateTime = _r[_f = 3].toInt<int64_t>();
			}
			catch(Exception const& /*_e*/)
			{
				cwarn << "dposcontext data populate error!";
			}
			//cdebug << "!!end streamRLPFields dposcontext ....";
		}
        void printData () const
		{
			DposContext const& _d = *this;
			std::cout << BrcYellow" DposData:" << BrcYellow;
			std::cout << BrcBlue "canlitor:{";
			for(auto val : _d.canlidates)
				std::cout << val << "| ";
			std::cout << "}" << std::endl << BrcBlue;

			std::cout << BrcBlue "varlitors_votes:{";
			for(auto val : _d.varlitor_votes)
			{
				std::cout << "[varlitor:" << val.first << "(";
				std::cout << val.second;
				std::cout << ")] ";
			}
			std::cout << "}" << std::endl << BrcBlue;

			std::cout << BrcBlue "vote_varlitors:{";
			for(auto val : _d.vote_varlitors)
			{
				std::cout << "[vote:" << val.first << "| varlitor:";
				std::cout << "(" << val.second <<")] ";
			}
			std::cout << "}" << std::endl << BrcYellow;
		}
        friend std::ostream& operator << (std::ostream& out , DposContext& _d)
		{
			out << "canlitor:{";
			for(auto val : _d.canlidates)
				out<< val << "| ";
			out << "}" << std::endl;

			out << "varlitors_votes:{";
			for(auto val : _d.varlitor_votes)
			{
				out << " [varlitor:" << val.first << " (";
				out << val.second;
				out << ") ]";
			}
			out << "}" << std::endl;

			std::cout << "vote_varlitors:{";
			for(auto val : _d.vote_varlitors)
			{
				std::cout << "[vote:" << val.first << "| varlitor:";
				std::cout << "(" << val.second << ")] ";
			}
			out << "}" << std::endl ;
            return out;
		}
    };

    }
}