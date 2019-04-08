#include "BRCTranscation.h"

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

bool dev::brc::BRCTranscation::verifyPendingOrder(
    Address const& _form, size_t _type, size_t _pendingOrderNum, size_t _pendingOrderPrice, h256 _pendingOrderHash)
{
    if (_type <= dev::brc::PendingOrderEnum::EPendingOrderNull ||
        _type >= dev::brc::PendingOrderEnum::EPendingOrderMax || _pendingOrderNum == 0 ||
        _pendingOrderPrice == 0)
    {
        return false;
    }

    if (_type == dev::brc::PendingOrderEnum::EBuyBrcPendingOrder)
    {
        if (_pendingOrderNum * _pendingOrderPrice > (size_t)m_state.balance(_form))
        {
            return false;
        }
    }
    else if (_type == dev::brc::PendingOrderEnum::ESellBrcPendingOrder)
    {
        if (_pendingOrderNum > (size_t)m_state.BRC(_form))
        {
            return false;
        }
    }
    else if (_type == dev::brc::PendingOrderEnum::EBuyFuelPendingOrder)
    {
        if (_pendingOrderNum * _pendingOrderPrice > (size_t)m_state.BRC(_form))
        {
            return false;
        }
    }
    else if (_type == dev::brc::PendingOrderEnum::ESellFuelPendingOrder)
	{
        if (_pendingOrderNum > (size_t)m_state.balance(_form))
		{
            return false;
		}
    }
    else if (_type == dev::brc::PendingOrderEnum::ECancelPendingOrder)
	{
		//TO DO :校验撤销挂单是否合法
	}

	return true;
}
