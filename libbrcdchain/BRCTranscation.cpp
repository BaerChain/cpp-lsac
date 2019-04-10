#include "BRCTranscation.h"
#include <brc/types.hpp>

bool dev::brc::BRCTranscation::verifyTranscation(
    Address const& _form, Address const& _to, size_t _type, size_t _transcationNum)
{
    if (_type <= dev::brc::TranscationEnum::ETranscationNull ||
        _type >= dev::brc::TranscationEnum::ETranscationMax || _transcationNum == 0)
    {
        return false;
    }

    if (_type == dev::brc::TranscationEnum::EBRCTranscation)
    {
        if (_form == _to)
        {
            return false;
        }
        if (_transcationNum > (size_t)m_state.BRC(_form))
        {
            return false;
        }
        return true;
    }
    else if (_type == dev::brc::TranscationEnum::EFBRCFreezeTranscation)
    {
        if (_form != _to)
        {
            return false;
        }
        if (_transcationNum > (size_t)m_state.BRC(_form))
        {
            return false;
        }
        return true;
    }
    else if (_type == dev::brc::TranscationEnum::EFBRCUnFreezeTranscation)
    {
        if (_form != _to)
		{
            return false;
		}
        if (_transcationNum > (size_t)m_state.FBRC(_form))
        {
            return false;
		}
        return true;
    }
    else if (_type == dev::brc::TranscationEnum::ECookieTranscation)
    {
        if (_form == _to || _to != Address() || _transcationNum == 0)
        {
            return false;
        }
        return true;
    }

    return false;
}

bool dev::brc::BRCTranscation::verifyPendingOrder(Address const& _form, exchange_plugin const& _exdb, int64_t _nowTime,  size_t _type,
    size_t _token_type, size_t _buy_type, size_t _pendingOrderNum, size_t& _pendingOrderPrice,
    h256 _pendingOrderHash)
{
    if (_type == brc::db::order_type::null_order ||
        _token_type == brc::db::order_token_type::null_token ||
        _buy_type == brc::db::order_buy_type::null_buy || _pendingOrderNum == 0 ||
        _pendingOrderPrice == 0)
    {
        return false;
    }

    try
    {
        std::map<u256, u256> _map = {_pendingOrderPrice, _pendingOrderNum};
        order _order = {_pendingOrderHash, _form, _buy_type, _token_type, _type, _map, _nowTime};
        _exdb.insert_operation(_order, true, true);
    }
    catch (const boost::exception& e)
    {
        ctrace << "verifyPendingOrder Error";
        return false;
    }
    return true;
}
