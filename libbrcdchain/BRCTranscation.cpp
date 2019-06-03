#include "BRCTranscation.h"
#include <brc/exchangeOrder.hpp>
#include <brc/types.hpp>

using namespace dev::brc::ex;

void dev::brc::BRCTranscation::verifyTranscation(
    Address const& _form, Address const& _to, size_t _type, const u256 & _transcationNum)
{
    if (_type <= dev::brc::TranscationEnum::ETranscationNull ||
        _type >= dev::brc::TranscationEnum::ETranscationMax || ( _type == dev::brc::TranscationEnum::EBRCTranscation && _transcationNum == 0))
    {
		BOOST_THROW_EXCEPTION(BrcTranscationField() 
							  << errinfo_comment("the brc transaction's type is error:" + toString(_type)));
    }

    if (_type == dev::brc::TranscationEnum::EBRCTranscation)
    {
        if (_form == _to)
        {
			BOOST_THROW_EXCEPTION(BrcTranscationField()
								  << errinfo_comment(" cant't transfer brc to me"));
        }
        if (_transcationNum > m_state.BRC(_form))
        {
			BOOST_THROW_EXCEPTION(BrcTranscationField()
								  << errinfo_comment(" not Enough brcd "));
        }
    }
	else if (_type == dev::brc::TranscationEnum::EAssetInjection)
	{
		return ;
	}
}

void dev::brc::BRCTranscation::verifyPendingOrder(Address const& _form, ex::exchange_plugin& _exdb,
    int64_t _nowTime, ex::order_type _type, ex::order_token_type _token_type, order_buy_type _buy_type, u256 _pendingOrderNum,
    u256 _pendingOrderPrice, u256 _transcationGas, h256 _pendingOrderHash)
{
    if ( _type == order_type::null_type ||
        (_buy_type == order_buy_type::only_price && (_type == order_type::buy || _type == order_type::sell) &&  (_pendingOrderNum == 0 || _pendingOrderPrice == 0)) ||
        (_buy_type == order_buy_type::all_price && ((_type == order_type::buy && (_pendingOrderNum != 0 || _pendingOrderPrice == 0)) ||
        (_type == order_type::sell && (_pendingOrderPrice != 0 || _pendingOrderNum == 0))))
        )
    {
         BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("Pending order type and parameters are incorrect")));
    }
 
	if( (_type != ex::order_type::sell && _type != ex::order_type::buy) || 
		(_token_type != ex::order_token_type::BRC && _token_type != ex::order_token_type::FUEL) || 
		(_buy_type != ex::order_buy_type::all_price && _buy_type != ex::order_buy_type::only_price))
	{
		BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("Pending order type and parameters are incorrect")));
	} 

    if (_type == order_type::buy && _token_type == order_token_type::BRC &&
        _buy_type == order_buy_type::only_price)
    {
        if (_pendingOrderNum * _pendingOrderPrice > m_state.BRC(_form))
        {
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("buy BRC only_price :Address BRC < Num * Price")));
        }
    }
    else if (_type == order_type::buy && _token_type == order_token_type::BRC &&
             _buy_type == order_buy_type::all_price)
    {
        if (_pendingOrderPrice > m_state.BRC(_form))
		{
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("buy BRC all_price :Address BRC < Price")));
		}
    }
    else if (_type == order_type::buy && _token_type == order_token_type::FUEL &&
			_buy_type == order_buy_type::only_price)
	{
        if (_pendingOrderNum * _pendingOrderPrice > m_state.balance(_form))
		{
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("buy Cookie only_price :Address Cookie < Num * Price")));
		}
	}
    else if (_type == order_type::buy && _token_type == order_token_type::FUEL &&
			_buy_type == order_buy_type::all_price)
	{
        if (_pendingOrderPrice > m_state.balance(_form))
		{
         BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("buy Cookie all_price :Address Cookie < Price")));
		}
    }
	else if (_type == order_type::sell && _token_type == order_token_type::BRC &&
		(_buy_type == order_buy_type::only_price || _buy_type == order_buy_type::all_price))
	{
        if (_pendingOrderNum > m_state.BRC(_form))
		{
           BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("sell BRC all_price or only_price :Address BRC < Num")));
		}
    }
    else if (_type == order_type::sell && _token_type == order_token_type::FUEL &&
		(_buy_type == order_buy_type::only_price || _buy_type == order_buy_type::all_price))
	{
        if (_pendingOrderNum > m_state.balance(_form))
		{
           BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("buy BRC all_price or only_price :Address Cookie < Num")));
		}
	}

    order_type __type;
    if (_buy_type == order_buy_type::all_price)
    {
        if (_type == order_type::buy)
        {
            __type = order_type::sell;
        }else{
            __type = order_type::buy;
        }
        auto _find_token = _token_type == order_token_type::BRC ? order_token_type::FUEL : order_token_type::BRC;
        std::vector<exchange_order> _v = _exdb.get_order_by_type(__type,_find_token, 10);  

        if(_v.size() == 0)
        {
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("There is no order for the corresponding order pool!")));
        }   
    }

	if(_type == order_type::buy && _token_type == order_token_type::BRC && _buy_type == order_buy_type::all_price)
	{
		try
		{
			std::map<u256, u256> _map = { {_pendingOrderPrice, _pendingOrderNum} };
			order _order = { _pendingOrderHash, _form, (order_buy_type)_buy_type,
				(order_token_type)_token_type, (order_type)_type, _map, _nowTime };
			const std::vector<order> _v = { {_order} };
			std::vector<result_order> _retV = _exdb.insert_operation(_v, true, true);

			u256 _cookieNum = 0;
			for(auto it : _retV)
			{
				_cookieNum += it.amount;
			}

			if(_cookieNum < _transcationGas)
			{
				BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("pendingorderFailed : The exchanged cookies are not enough to pay the commission!")));
			}
		}
		catch(const boost::exception& e)
		{
			BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("pendingorderFailed : buy BRC allprice is failed!")));
			cwarn << "verifyPendingOrder Error " << boost::diagnostic_information(e);
		}
		catch(...)
		{
			BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("pendingorderFailed : buy BRC allprice unkonwn failed!")));
			return;
		}
	}
}


void dev::brc::BRCTranscation::verifyCancelPendingOrder(ex::exchange_plugin& _exdb, Address _addr, h256 _hash)
{
	if (_hash == h256(0))
	{
		BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(std::string("Pendingorder hash cannot be 0")));
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
		BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(std::string("This order does not exist in the trading pool")));
	}

	for (auto val : _resultV)
	{
		if (_addr != val.sender)
		{
			BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(std::string("This order is not the same as the transaction sponsor account")));
		}
	}
}
