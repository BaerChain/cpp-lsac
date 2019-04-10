#include "BRCTranscation.h"
#include <brc/exchangeOrder.hpp>
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

bool dev::brc::BRCTranscation::verifyPendingOrder(Address const& _form,
    ex::exchange_plugin const& _exdb, int64_t _nowTime, int _type, int _token_type, int _buy_type,
    u256 _pendingOrderNum, u256& _pendingOrderPrice, h256 _pendingOrderHash)
{
    if (_type == order_type::null_type || _pendingOrderNum == 0 || _pendingOrderPrice == 0)
    {
        return false;
    }

    try
    {
        std::map<u256, u256> _map = {_pendingOrderPrice, _pendingOrderNum};
        order _order = {_pendingOrderHash, _form, (order_buy_type)_buy_type, (order_token_type)_token_type, (order_type)_type, _map, _nowTime};
        std::vector<order> _v = {_order};
		_exdb.insert_operation(_v, true, true);
    }
    catch (const boost::exception& e)
    {
        ctrace << "verifyPendingOrder Error";
        return false;
    }
    return true;
}
