#ifndef BEARCHAIN_EXCHANGETYPE_H
#define BEARCHAIN_EXCHANGETYPE_H

#include <libdevcore/Common.h>
#include <libdevcore/Address.h>
#include <libdevcore/RLP.h>
#include <brc/objects.hpp>
#include <indexDb/database/include/brc/types.hpp>


namespace dev{
    
    namespace brc{

        struct  exchangeSort
        {
            u256 m_exchangePrice;
            int64_t m_exchangeTime;

            bool operator<(exchangeSort const& _e) const
            {
                if(m_exchangePrice < _e.m_exchangePrice)
                {
                    return true;
                }else if(m_exchangePrice == _e.m_exchangePrice)
                {
                    if(m_exchangeTime < _e.m_exchangeTime)
                    {
                        return true;
                    }
                }
                return false;
            }

            bool operator>(exchangeSort const& _e) const
            {
                if(m_exchangePrice > _e.m_exchangePrice)
                {
                    return true;
                }else if(m_exchangePrice == _e.m_exchangePrice)
                {
                    if(m_exchangeTime < _e.m_exchangeTime)
                    {
                        return true;
                    }
                }
                return false;
            }

            void encode(RLPStream &rlp) const
            {
                rlp.appendList(2);
                rlp.append(m_exchangePrice);
                rlp.append(m_exchangeTime);
            }

            void decode(RLP const& _rlp)
            {
                if(_rlp.isList())
                {
                    m_exchangePrice = _rlp[0].toInt<u256>();
                    m_exchangeTime = _rlp[1].toInt<int64_t>();
                }
            } 
            std::string to_string() const
            {
                return " price = " + toJS(m_exchangePrice) + " time = " + toJS(m_exchangeTime);
            }
        };

        struct exchangeValue
        {
            h256 m_orderId;
            Address m_from;
            u256 m_pendingorderNum = 0;
            u256 m_pendingordertokenNum = 0;
            u256 m_pendingorderPrice = 0;
            int64_t m_createTime;
            ex::order_type m_pendingorderType = ex::order_type::null_type;
            ex::order_token_type m_pendingorderTokenType = ex::order_token_type::BRC;
            ex::order_buy_type m_pendingorderBuyType = ex::order_buy_type::all_price;

            void encode(RLPStream &_rlp) const
            {   
                _rlp.appendList(9);
                _rlp.append(m_orderId);
                _rlp.append(m_from);
                _rlp.append(m_pendingorderNum);
                _rlp.append(m_pendingordertokenNum);
                _rlp.append(m_pendingorderPrice);
                _rlp.append(m_createTime);
                _rlp.append((uint8_t)m_pendingorderType);
                _rlp.append((uint8_t)m_pendingorderTokenType);
                _rlp.append((uint8_t)m_pendingorderBuyType); 
            }

            void decode(RLP const& _rlp) 
            {
                std::cout << "decode " << std::endl; 
                if(_rlp.isList())
                {
                    m_orderId = _rlp[0].convert<h256>(RLP::LaissezFaire);
                    m_from = _rlp[1].convert<Address>(RLP::LaissezFaire);
                    m_pendingorderNum = _rlp[2].convert<u256>(RLP::LaissezFaire);
                    m_pendingordertokenNum = _rlp[3].convert<u256>(RLP::LaissezFaire);
                    m_pendingorderPrice = _rlp[4].convert<u256>(RLP::LaissezFaire);
                    m_createTime = _rlp[5].toInt<int64_t>();
                    m_pendingorderType = (ex::order_type)_rlp[6].convert<uint8_t>(RLP::LaissezFaire);
                    m_pendingorderTokenType = (ex::order_token_type)_rlp[7].convert<uint8_t>(RLP::LaissezFaire);
                    m_pendingorderBuyType = (ex::order_buy_type)_rlp[8].convert<uint8_t>(RLP::LaissezFaire);
                }
            }

            std::string to_string() const
            {
                return " orderId = " + toJS(m_orderId);
            }

            ex::ex_order to_ex_order() const{
                ex::ex_order order;
                order.trxid =  m_orderId;
                order.sender =  m_from;
                order.price =  m_pendingorderPrice;
                order.token_amount =  m_pendingordertokenNum;
                order.source_amount =  m_pendingorderNum;
                order.create_time =  m_createTime;
                order.type =  m_pendingorderType;
                order.token_type =  m_pendingorderTokenType;
                order.buy_type =  m_pendingorderBuyType;
                return order;
            }
        };
        
        
    }

}



#endif