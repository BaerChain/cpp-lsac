#include <libdevcore/Address.h>
#include <libbrccore/Common.h>
#include <jsoncpp/json/value.h>

namespace dev
{
    struct accountStu
    {
        accountStu(){}
        accountStu(Address _addr, u256 _cookie, u256 _fcookie, u256 _brc, u256 _fbrc, u256 _voteAll,
            u256 _ballot, u256 _poll, u256 _nonce, u256 _cookieInSummury, std::map<Address, u256> _vote):
            m_addr(_addr),
            m_cookie(_cookie),
            m_fcookie(_fcookie),
            m_brc(_brc),
            m_fbrc(_fbrc),
            m_voteAll(_voteAll),
            m_ballot(_ballot),
            m_poll(_poll),
            m_nonce(_nonce),
            m_cookieInSummury(_cookieInSummury),
            m_vote(_vote)
        {}
        
        private:
        Address m_addr;
        u256 m_cookie;
        u256 m_fcookie;
        u256 m_brc;
        u256 m_fbrc;
        u256 m_voteAll;
        u256 m_ballot;
        u256 m_poll;
        u256 m_nonce;
        u256 m_cookieInSummury;
        std::map<Address,u256> m_vote;
    };  

    struct VoteStu
    {
        VoteStu(){}
        VoteStu(std::map<Address, u256> _vote, u256 _totalVoteNum):
            m_vote(_vote),
            m_totalVoteNum(_totalVoteNum){}
        
        
        private:
            std::map<Address, u256> m_vote;
            u256 m_totalVoteNum;
    };

    struct electorStu
    {
        electorStu(){}
        electorStu(std::map<Address, u256> _elector):
            m_elector(_elector){}

        private:
            std::map<Address, u256> m_elector;
    };

} 


