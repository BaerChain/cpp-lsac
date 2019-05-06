#include "BRCTranscation.h"
#include <brc/exchangeOrder.hpp>
#include <brc/types.hpp>

using namespace dev::brc::ex;

bool dev::brc::BRCTranscation::verifyTranscation(
    Address const& _form, Address const& _to, size_t _type, size_t _transcationNum)
{
    if (_type <= dev::brc::TranscationEnum::ETranscationNull ||
        _type >= dev::brc::TranscationEnum::ETranscationMax || ( _type == dev::brc::TranscationEnum::EBRCTranscation && _transcationNum == 0))
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
	else if (_type == dev::brc::TranscationEnum::EAssetInjection)
	{
		return true;
	}

    return false;
}

bool dev::brc::BRCTranscation::verifyPendingOrder(Address const& _form, ex::exchange_plugin& _exdb,
    int64_t _nowTime, ex::order_type _type, ex::order_token_type _token_type, order_buy_type _buy_type, u256 _pendingOrderNum,
    u256 _pendingOrderPrice, h256 _pendingOrderHash)
{
    if ( _type == order_type::null_type ||
        (_buy_type == order_buy_type::only_price && (_type == order_type::buy || _type == order_type::sell) &&  (_pendingOrderNum == 0 || _pendingOrderPrice == 0)) ||
        (_buy_type == order_buy_type::all_price && ((_type == order_type::buy && (_pendingOrderNum != 0 || _pendingOrderPrice == 0)) ||
        (_type == order_type::sell && (_pendingOrderPrice != 0 || _pendingOrderNum == 0))))
        )
    {
        return false;
    }

    if (_type == order_type::buy && _token_type == order_token_type::BRC &&
        _buy_type == order_buy_type::only_price)
    {
        if (_pendingOrderNum * _pendingOrderPrice > m_state.BRC(_form))
        {
            return false;
        }
    }
    else if (_type == order_type::buy && _token_type == order_token_type::BRC &&
             _buy_type == order_buy_type::all_price)
    {
        if (_pendingOrderPrice > m_state.BRC(_form))
		{
            return false;
		}
    }
    else if (_type == order_type::buy && _token_type == order_token_type::FUEL &&
			_buy_type == order_buy_type::only_price)
	{
        if (_pendingOrderNum * _pendingOrderPrice > m_state.balance(_form))
		{
            return false;
		}
	}
    else if (_type == order_type::buy && _token_type == order_token_type::FUEL &&
			_buy_type == order_buy_type::all_price)
	{
        if (_pendingOrderPrice > m_state.balance(_form))
		{
            return false;
		}
    }
	else if (_type == order_type::sell && _token_type == order_token_type::BRC &&
		(_buy_type == order_buy_type::only_price || _buy_type == order_buy_type::all_price))
	{
        if (_pendingOrderNum > m_state.BRC(_form))
		{
            return false;
		}
    }
    else if (_type == order_type::sell && _token_type == order_token_type::FUEL &&
		(_buy_type == order_buy_type::only_price || _buy_type == order_buy_type::all_price))
	{
        if (_pendingOrderNum > m_state.balance(_form))
		{
            return false;
		}
	}

//    try
//    {
//        std::map<u256, u256> _map = {{_pendingOrderPrice, _pendingOrderNum}};
//        order _order = {_pendingOrderHash, _form, (order_buy_type)_buy_type,
//            (order_token_type)_token_type, (order_type)_type, _map, _nowTime};
//        const std::vector<order> _v = {{_order}};
//        _exdb.insert_operation(_v, true, true);
//    }
//    catch (const boost::exception& e)
//    {
//        cwarn << "verifyPendingOrder Error " << boost::diagnostic_information(e);
//        return false;
//    }
//    catch (...){
//        cwarn << "unkown exception .";
//        return false;
//    }
    return true;
}


bool dev::brc::BRCTranscation::verifyCancelPendingOrder(ex::exchange_plugin& _exdb, Address _addr, h256 _hash)
{
	if (_hash == h256(0))
	{
		return false;
	}

	std::vector <ex::order> _resultV;
	try
	{
		const std::vector<h256> _HashV = { {_hash} };
		_resultV = _exdb.cancel_order_by_trxid(_HashV, true);
	}
	catch (const boost::exception& e)
	{
		cwarn << "cancelpendingorder error" << boost::diagnostic_information(e);
		return false;
	}

	for (auto val : _resultV)
	{
		if (_addr != val.sender)
		{
			return false;
		}
	}

	return true;
}
