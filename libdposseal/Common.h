#pragma once
/*
    dpos 相关数据结构
*/
#include <iostream>
#include <libdevcore/concurrent_queue.h>
#include <libdevcore/RLP.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Exceptions.h>
#include <libp2p/Common.h>
#include <libdevcore/FixedHash.h>
#include <libethcore/Common.h>
#include <libethcore/DposData.h>
#include <libdevcore/Log.h>

namespace dev
{
    namespace bacd
    {
        using NodeID = p2p::NodeID;
        using DposContext = eth::DposContext;
        const unsigned int epochInterval = 120000;         // 一个出块轮询周期 ms
		const unsigned int varlitorInterval = 1000;        // 一个出块人一次出块时间
        const unsigned int blockInterval = 1000;           // 一个块最短出块时间 ms 
        const unsigned int valitorNum = 5;              // 筛选验证人的人数最低值
        const unsigned int maxValitorNum = 21;          // 最大验证人数量
        const unsigned int verifyVoteNum = 6;           // 投票交易确认数

        enum DposPacketType :byte
        {
            DposStatuspacket = 0x23,
            DposDataPacket,
            DposPacketCount
        };
        enum EDposDataType
        {
            e_loginCandidate =0,         // 成为候选人
            e_logoutCandidate,
            e_delegate,            // 推举验证人
            e_unDelegate,

            e_max
        };
        enum EDPosResult
        {
            e_Add =0,
            e_Dell,
            e_Full,
            e_Max
        };
        //统计投票数据结构
        struct DposVarlitorVote
        {
            Address     m_addr;
            size_t      m_vote_num;
            DposVarlitorVote(Address const& _addr, size_t _num)
            {
                m_addr = _addr;
                m_vote_num = _num;
            }
            bool operator >= (DposVarlitorVote const& _d) { return m_vote_num >= _d.m_vote_num; }
            bool operator < (DposVarlitorVote const& _d) { return m_vote_num < _d.m_vote_num; }
        };

        /***************************交易投票相关数据 start*********************************************/
        struct DposTransaTionResult
        {
            h256            m_hash;
            EDposDataType   m_type;
            size_t          m_epoch;            //交易目标是第几轮投票
            Address         m_send_to;
            Address         m_form;
            size_t          m_block_hight;      //所在区块的高度
            DposTransaTionResult()
            {
                m_hash = h256();
                m_type = EDposDataType::e_max;
                m_epoch = 0;
                m_send_to = Address();
                m_form = Address();
                m_block_hight = 0;
            }
            bool operator < (DposTransaTionResult const& _d) const { return m_block_hight < _d.m_block_hight; }
            friend std::ostream& operator << (std::ostream& out, DposTransaTionResult const& _d)
            {
                out <<"{"<< std::endl;
                out <<"hash:" << _d.m_hash << std::endl;
                out<< "type:" << _d.m_type << std::endl;
                out<<"epoch:" << _d.m_epoch << std::endl;
                out<<"from:" << _d.m_form << std::endl;
                out<<"sender_to:" << _d.m_send_to << std::endl;
                out << "block_hight" << _d.m_block_hight << std::endl;
                return out;
            }
            void operator = (DposTransaTionResult const& _d)
            {
                m_hash = _d.m_hash;
                m_type = _d.m_type;
                m_epoch = _d.m_epoch;
                m_send_to = _d.m_send_to;
                m_form = _d.m_form;
                m_block_hight = _d.m_block_hight;
            }
        };
        struct OnDealTransationResult : DposTransaTionResult
        {
            EDPosResult     m_ret_type;
            bool            m_is_deal_vote;

            OnDealTransationResult() :DposTransaTionResult(), m_ret_type(EDPosResult::e_Max), m_is_deal_vote(false) { }
            OnDealTransationResult(EDPosResult type, bool _is_deal, DposTransaTionResult const& _d) :
                DposTransaTionResult(_d),
                m_ret_type(type),
                m_is_deal_vote(_is_deal)
            {
            }

            friend std::ostream& operator <<(std::ostream& out, OnDealTransationResult const& _ret)
            {
                out << static_cast<DposTransaTionResult>(_ret) 
                    << std::endl<< "EDPosResult:" << _ret.m_ret_type 
                    << std::endl << "m_is_deal_vote:" << _ret.m_is_deal_vote;
                return out;
            }
            void streamRLPFields(RLPStream& _s) const
            {
                _s.appendList(8);
                _s << m_hash.asBytes() << (size_t)m_type << m_epoch << 
					m_send_to.asBytes() << m_form.asBytes() << 
					m_block_hight << (size_t)m_ret_type << (size_t)m_is_deal_vote;
            }
            void populate(RLP const& _rlp)
            {
                size_t field = 0;
                try
                {
                    
                    m_hash =h256(_rlp[field = 0].toBytes());
                    m_type = (EDposDataType)_rlp[field = 1].toInt<size_t>();
                    m_epoch = _rlp[field = 2].toInt<size_t>();
                    m_send_to =Address(_rlp[field = 3].toBytes());
                    m_form =Address(_rlp[field = 4].toBytes());
                    m_block_hight = _rlp[field = 5].toInt<size_t>();
					m_ret_type = (EDPosResult)_rlp[field = 6].toInt<size_t>();
					m_is_deal_vote = (bool)_rlp[field = 7].toInt<size_t>();
                }
                catch(Exception const& /*_e*/)
                {
                    /*_e << errinfo_name("invalid msg format") << BadFieldError(field, toHex(_rlp[field].data().toBytes()));
                    throw;*/
                    cwarn << "populate OnDealTransationResult is error";
                }
            }
        };

        /***************************交易投票相关数据 end*********************************************/

        /***************************网络数据包 封装业务数据 start*********************************************/
        struct DposMsgPacket
        {
            h512        node_id;     // peer id
            unsigned    packet_id;   //msg id
            bytes       data;        //rlp data
            DposMsgPacket() :node_id(h512(0)), packet_id(0) { }
            DposMsgPacket(h512 _id, unsigned _pid, bytesConstRef _data)
                :node_id(_id), packet_id(_pid), data(_data.toBytes())
            { }
        };

        using DposMsgQueue = dev::concurrent_queue<DposMsgPacket>;

        struct DPosStatusMsg
        {
            Address        m_addr;
            int64_t        m_now;
            DPosStatusMsg() : m_addr(Address()) { }
            virtual void streamRLPFields(RLPStream& _s) const
            {
                //_s.appendList(2);
                _s << m_addr.asBytes() << m_now;
            }
            virtual void populate(RLP const& _rlp)
            {
                size_t field = 0;
                try
                {
                    m_addr =Address(_rlp[field = 0].toBytes());
                    m_now = _rlp[field = 1].toInt<int64_t>();
                }
                catch (Exception const& /*_e*/)
                {
                    /*_e << errinfo_name("invalid msg format") << BadFieldError(field, toHex(_rlp[field].data().toBytes()));
                    throw;*/
                    cwarn << "populate DPosStatusMsg is error feild :" << field;
                }
            }

            friend std::ostream& operator << (std::ostream& out, DPosStatusMsg const& _d)
            {
                out<< "{"<<
                    std::endl << "Address:" << _d.m_addr <<
                    std::endl << "|time:" << _d.m_now ;
                return out;
            }
        };

        struct DposDataMsg:DPosStatusMsg
        {
            std::vector<OnDealTransationResult>  m_transation_ret;        //OnDealTransationResult 数据
            DposDataMsg() :DPosStatusMsg()
            {
                m_transation_ret.clear();
            }
            void streamRLPFields(RLPStream& _s) const override
            {
                //_s.appendList(3);
                DPosStatusMsg::streamRLPFields(_s);
                std::vector<bytes> temp_byte;
                for(auto val : m_transation_ret)
                {
                    RLPStream rlps;
                    val.streamRLPFields(rlps);
                    temp_byte.push_back(rlps.out());
                }
                _s.appendVector<bytes>(temp_byte);
            }
            void populate(RLP const& _rlp) override
            {
                size_t field = 0;
                DPosStatusMsg::populate(_rlp);
                try
                {
                    std::vector<bytes> temp_byte = _rlp[field = 2].toVector<bytes>();
                    for(auto val: temp_byte)
                    {
                        OnDealTransationResult ret;
                        RLP _r(val);
                        ret.populate(_r);
                        m_transation_ret.push_back(ret);
                    }
                }
                catch(Exception const& /*_e*/)
                {
                    /*_e << errinfo_name("invalid msg format") << BadFieldError(field, toHex(_rlp[field].data().toBytes()));
                    throw;*/
                    cwarn << "populate DposDataMsg is error field: "<< field;
                }
            }

            friend std::ostream& operator << (std::ostream& out , DposDataMsg const& _d)
            {
                out << static_cast<DPosStatusMsg>(_d) << std::endl;
                out << "OnDealTransationResult: {"<<std::endl;
                for( auto val: _d.m_transation_ret)
                {
                    out << val << " "<<std::endl;
                }
                out << " }"<< std::endl;
                return out;
            }
        };

        /***************************网络数据包 封装业务数据 start*********************************************/
    }
}