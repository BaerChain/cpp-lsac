#include "BRCTranscation.h"
#include "DposVote.h"
#include "ExdbState.h"
#include <brc/exchangeOrder.hpp>
#include <brc/types.hpp>
#include <libbrccore/config.h>
using namespace dev::brc::ex;

#define VOTETIME 60*1000
#define VOTEBLOCKNUM 100

#define SELLCOOKIELIMIT 1000000000000
#define BUYCOOKIELIMIT 1000000000000
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
}



void dev::brc::BRCTranscation::verifyPendingOrders(Address const& _form, u256 _total_cost, ex::exchange_plugin& _exdb,
												   int64_t _nowTime, u256 _transcationGas, h256 _pendingOrderHash,
												   std::vector<std::shared_ptr<transationTool::operation>> const& _ops,
                                                   int64_t _blockHeight) {

    u256 total_brc = 0;
    u256 total_cost = _total_cost;
    std::vector<ex_order> _verfys;

    for (auto const &val :_ops) {
        std::shared_ptr<transationTool::pendingorder_opearaion> pen = std::dynamic_pointer_cast<transationTool::pendingorder_opearaion>(
                val);
        if (!pen)
            BOOST_THROW_EXCEPTION(
                    VerifyPendingOrderFiled() << errinfo_comment(std::string("Pending order type is incorrect!")));
        order_type __type = (order_type) pen->m_Pendingorder_type;
        ex::order_type _type = pen->m_Pendingorder_type;
        ex::order_token_type _token_type = pen->m_Pendingorder_Token_type;
        order_buy_type _buy_type = pen->m_Pendingorder_buy_type;
        u256 _pendingOrderNum = pen->m_Pendingorder_num;
        u256 _pendingOrderPrice = pen->m_Pendingorder_price;


        if (_type == order_type::null_type || _token_type == order_token_type::BRC ||
            (_buy_type == order_buy_type::only_price && (_type == order_type::sell || _type == order_type::buy) &&
             (_pendingOrderNum == 0 || _pendingOrderPrice == 0)) ||
            (_buy_type == order_buy_type::all_price &&
             ((_type == order_type::buy && (_pendingOrderNum != 0 || _pendingOrderPrice == 0)) ||
              (_type == order_type::sell && (_pendingOrderPrice != 0 || _pendingOrderNum == 0))))
                ) {
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                    std::string("Pending order type and parameters are incorrect")));
        }

        if ((_type != ex::order_type::sell && _type != ex::order_type::buy) ||
            _token_type != ex::order_token_type::FUEL ||
            (_buy_type != ex::order_buy_type::all_price && _buy_type != ex::order_buy_type::only_price)) {
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                    std::string("Pending order type and parameters are incorrect")));
        }


        if (_pendingOrderNum % 10000 > 0 || _pendingOrderPrice % 10000 > 0) {
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                    std::string("Pending order accuracy cannot be greater than 4 digits accuracy")));
        }

        if (_type == order_type::buy) {
            if (_buy_type == order_buy_type::only_price) {
                total_brc += _pendingOrderNum * _pendingOrderPrice / PRICEPRECISION;
                if (total_brc > m_state.BRC(_form)) {
                    BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                            std::string("buy Cookie only_price :Address BRC < Num * Price")));
                }
                if(total_brc < BUYCOOKIELIMIT)
                {
                    BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                            std::string("Limit orders to buy cookies can not be less than 10000 BRC")));
                }
            } else if (_buy_type == order_buy_type::all_price) {
                total_brc += _pendingOrderPrice;
                if (total_brc > m_state.BRC(_form)) {
                    BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                            std::string("buy Cookie all_price :Address BRC < Num * Price")));
                }
            }
        } else if (_type == order_type::sell) {
            if (_buy_type == order_buy_type::only_price || _buy_type == order_buy_type::all_price) {
                total_cost += _pendingOrderNum;
                if (total_cost > m_state.balance(_form)) {
                    BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                            std::string("sell Cookie only_price :Address balance < Num * Price")));
                }
                if(total_cost < SELLCOOKIELIMIT && _buy_type == order_buy_type::only_price)
                {
                    BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                            std::string("Limit sell orders can not sell less than 10000 COOKIE")));
                }
            }
        }


        ExdbState _exdbState(m_state);
        if (_buy_type == order_buy_type::all_price) {
            if (_type == order_type::buy) {
                __type = order_type::sell;
            } else if (_type == order_type::sell) {
                __type = order_type::buy;
            }
            std::vector<exchange_order> _v = _exdbState.get_order_by_type(__type, order_token_type::FUEL, 10);

            if (_v.size() == 0) {
                BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(
                        std::string("There is no order for the corresponding order pool!")));
            }
        }

        if (_type == order_type::buy && _token_type == order_token_type::FUEL &&
            _buy_type == order_buy_type::all_price) {
            std::pair<u256, u256> _pair = {_pendingOrderPrice * PRICEPRECISION, _pendingOrderNum};
            ex_order _order = {_pendingOrderHash, _form, _pendingOrderPrice * PRICEPRECISION, _pendingOrderNum,
                               _pendingOrderNum, _nowTime, (order_type) _type, (order_token_type) _token_type,
                               (order_buy_type) _buy_type};
            _verfys.push_back(_order);
        }
    }


    if (_verfys.empty())
        return;
    for (auto _val : _verfys)
    {
        try{
            ExdbState _exdbState(m_state);
            std::vector<result_order> _retV = _exdbState.insert_operation(_val, true);
            u256 _cookieNum = 0;
            for(auto it : _retV){
                if(it.type == order_type::buy && it.token_type == order_token_type::FUEL && it.buy_type == order_buy_type::all_price){
                    _cookieNum += it.amount;
                }
            }
            if( (_cookieNum + m_state.balance(_form)) < _transcationGas){
                BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("pendingorderFailed : The exchanged cookies are not enough to pay the commission!")));
            }
        }
        catch(const boost::exception& e){
            cwarn << "verifyPendingOrder Error " << boost::diagnostic_information(e);
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("pendingorderFailed : buy BRC allprice is failed!")));
        }
        catch(...){
            BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(std::string("pendingorderFailed : buy BRC allprice unkonwn failed!")));
        }
    }

}


void dev::brc::BRCTranscation::verifyCancelPendingOrders(ex::exchange_plugin & _exdb, Address _addr, std::vector<std::shared_ptr<transationTool::operation>> const & _ops){
	std::vector<h256> _HashV;
	for(auto const& val : _ops){
		std::shared_ptr<transationTool::cancelPendingorder_operation> can_order = std::dynamic_pointer_cast<transationTool::cancelPendingorder_operation>(val);
        if(!can_order)
			BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(std::string("Pendingorder type is error!")));
        if(can_order->m_hash == h256(0))
			BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(std::string("Pendingorder hash cannot be 0")));
		_HashV.push_back(can_order->m_hash);
	}
    ExdbState _exdbState(m_state);
	std::vector <ex::ex_order> _resultV;

    try{
        for(auto _it : _HashV)
        {
            _resultV = _exdbState.exits_trxid(_it);
        }
	}
	catch(const boost::exception& e){
		cwarn << "cancelpendingorder error" << boost::diagnostic_information(e);
		BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(std::string("This order does not exist in the trading pool")));
	}
	if(_resultV.size() == 0){
        BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(std::string("This order does not exist in the trading pool  . vertiy")));
	}


	for(auto val : _resultV){
        if (_addr != val.sender)
        {
            BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(std::string("This order is not the same as the transaction sponsor account")));
        }
	}
}

void dev::brc::BRCTranscation::verifyreceivingincome(dev::Address const& _from, std::vector<std::shared_ptr<transationTool::operation>> const& _ops,dev::brc::transationTool::dividendcycle _type, dev::brc::EnvInfo const& _envinfo, dev::brc::DposVote const& _vote)
{
    for(auto const& _it : _ops)
    {
        std::shared_ptr<transationTool::receivingincome_operation> _op =  std::dynamic_pointer_cast<transationTool::receivingincome_operation>(_it);
        if(!_op)
        {
            BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("receivingincome_operation is error")));
        }

        ReceivingType _receType = (ReceivingType)_op->m_receivingType;
        if(_receType == ReceivingType::RBlockFeeIncome)
        {
            verifyBlockFeeincome(_from, _envinfo, _vote);
        }else if(_receType == ReceivingType::RPdFeeIncome)
        {
            verifyPdFeeincome(_from, _envinfo.number(), _vote);
        }else{
            BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("receivingincome type is null")));
        }
    }
}

void dev::brc::BRCTranscation::verifyBlockFeeincome(dev::Address const& _from, const dev::brc::EnvInfo &_envinfo,
                                                    const dev::brc::DposVote &_vote) {
    std::pair<uint32_t, Votingstage> _pair = config::getVotingCycle(_envinfo.number());
    if (_pair.second == Votingstage::ERRORSTAGE) {
        BOOST_THROW_EXCEPTION(
                receivingincomeFiled() << errinfo_comment(std::string("There is currently no income to receive")));
    }

    auto a = m_state.account(_from);
    if (!a) {
        BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(
                std::string("The account that receives the income does not exist")));
    }

    auto sysAccount = m_state.account(SysVarlitorAddress);
    if (!sysAccount)
    {
        BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("Unable to get system account instance")));
    }

    std::pair<bool, u256> ret_pair = a->get_no_record_snapshot((u256) _pair.first, _pair.second);
    VoteSnapshot _voteSnapshot;
    if (ret_pair.first)
        _voteSnapshot = a->try_new_temp_snapshot(ret_pair.second);
    else
        _voteSnapshot = a->vote_snashot();
//    u256 _numberofrounds = _voteSnapshot.numberofrounds;
    if (_voteSnapshot.m_voteDataHistory.size() == 0 && a->vote_data().size() == 0)
    {
        BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("no votedataHistory: There is currently no income to receive")));
    }

    ReceivedCookies _receivedcookie = a->get_received_cookies();
    u256 _numberofround = config::getvoteRound(_receivedcookie.m_numberofRound);
    std::map<u256, std::vector<PollData>> _minerSnap =  m_state.get_miner_snapshot();
    std::map<u256, std::map<Address, u256>>::const_iterator _voteIt = _voteSnapshot.m_voteDataHistory.find(_numberofround - 1);
    if(_voteIt == _voteSnapshot.m_voteDataHistory.end())
    {
        std::vector<PollData> _dataV = a->vote_data();
        bool _status = false;
        std::vector<PollData> _mainNodeAddr = sysAccount->vote_data();
        if(isMainNode(_dataV, _mainNodeAddr))
        {
            _status = true;
        }
        if(_status == false)
        {
            BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("isMainNode fasle :The node that this account votes does not have a super node")));
        }
    }else {
        bool _status = false;

        for (; _voteIt != _voteSnapshot.m_voteDataHistory.end(); _voteIt++)
        {
            if(_minerSnap.count(_voteIt->first))
            {
                std::vector<PollData> _mainNodeV = _minerSnap[_voteIt->first];
                if(isMainNode(_voteIt->second, _mainNodeV))
                {
                    _status = true;
                }
            }else{
                std::vector<PollData> _nowMiner = m_state.vote_data(SysVarlitorAddress);
                if(isMainNode(_voteIt->second, _nowMiner))
                {
                    _status = true;
                }
            }
        }

        if(_status == false)
        {
            BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string(" voteDataSnapshot end: The node that this account votes does not have a super node")));
        }
    }
}

void dev::brc::BRCTranscation::verifyPdFeeincome(dev::Address const& _from, int64_t _blockNum, dev::brc::DposVote const& _vote)
{
    std::pair <uint32_t, Votingstage> _pair = config::getVotingCycle(_blockNum);
    if(_pair.second == Votingstage::ERRORSTAGE)
    {
        BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("There is currently no income to receive")));
    }

    auto a = m_state.account(_from);
    auto systemAccount = m_state.account(dev::PdSystemAddress);
    auto miners = m_state.account(dev::SysVarlitorAddress);
    if(!a || !miners)
    {
        BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("The account that receives the income does not exist")));
    }

    u256 _rounds = systemAccount->getSnapshotRounds();
    u256 _numofRounds = a->getFeeNumofRounds();
    std::map<u256, std::vector<PollData>> _map = systemAccount->getPollDataSnapshot();

    VoteSnapshot _voteSnapshot;
    std::pair<bool, u256> ret_pair = a->get_no_record_snapshot((u256) _pair.first, _pair.second);
    if (ret_pair.first)
        _voteSnapshot = a->try_new_temp_snapshot(ret_pair.second);
    else
        _voteSnapshot = a->vote_snashot();

    bool  is_received = false;
    for(int i= (int)_numofRounds ; i< _pair.first; i++){
        if (_voteSnapshot.m_voteDataHistory.count(i-1) && _map.count(i)){
            for(auto const& val: _map[i]){
                if (_voteSnapshot.m_voteDataHistory[i-1].count(val.m_addr) && _voteSnapshot.m_voteDataHistory[i-1][val.m_addr] !=0){
                    is_received = true;
                    return;
                }
                if (_from == val.m_addr){
                    is_received = true;
                    return;
                }
            }
        }
        if (is_received)
            break;
    }

    uint32_t  num = config::minner_rank_num();
    if (_voteSnapshot.m_voteDataHistory.count(_pair.first-1)) {
        for (auto const &val: miners->vote_data()) {
            if (num <= 0)
                break;
            --num;
            if (_voteSnapshot.m_voteDataHistory[_pair.first-1].count(val.m_addr)){
                is_received = true;
                break;
            }
        }
    }
    num = config::minner_rank_num();
    for(auto const& val: miners->vote_data()){
        if (num <=0)
            break;
        --num;
        if (_from == val.m_addr){
            is_received = true;
            break;
        }
    }

    if(is_received == false)
    {
        BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(std::string("This account does not meet the redemption fee")));
    }
}

void dev::brc::BRCTranscation::verifyTransferAutoEx(const dev::Address &_from,
                                                    const std::vector<std::shared_ptr<dev::brc::transationTool::operation>> &_op, u256 const& _baseGas, h256 const& _trxid, dev::brc::EnvInfo const& _envinfo)
{
    int64_t const& _timeStamp = _envinfo.timestamp();
    if((_envinfo.number() < 4072941 && _envinfo.header().chain_id() == 0x1)
        || (_envinfo.number() < 99999999 && _envinfo.header().chain_id() == 0xb))
    {
        BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("Transfer automatic exchange fee function has not yet reached the opening time")));
    }
    for(auto const& _opIt : _op)
    {
        std::shared_ptr<transationTool::transferAutoEx_operation> _autoExop =  std::dynamic_pointer_cast<transationTool::transferAutoEx_operation>(_opIt);
        if(_autoExop->m_from != _from)
        {
            BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("Initiating transaction address is inconsistent with operation address")));
        }
        if(_autoExop->m_autoExNum % 10000 > 0)
        {
            BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("Pending order accuracy cannot be greater than 4 digits accuracy")));
        }
        if(_autoExop->m_autoExType == transationTool::transferAutoExType::Balancededuction)
        {
            if(_autoExop->m_transferNum + _autoExop->m_autoExNum > m_state.BRC(_from))
            {
                BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("The number of accounts BRC is not enough to complete the transaction")));
            }
        }else if(_autoExop->m_autoExType == transationTool::transferAutoExType::Transferdeduction)
        {
            if(_autoExop->m_autoExNum > _autoExop->m_transferNum)
            {
                BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("The number of automatic redemptions is greater than the number of transfers")));
            }
            if(_autoExop->m_transferNum > m_state.BRC(_from))
            {
                BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("The number of transfer BRC is greater than the account balance")));
            }
        }else{
            BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("Automatically redeem unknown exceptions")));
        }
        
        std::vector<ex::result_order> _result;
        try
        {
            ExdbState _exdbState(m_state);
            ex::ex_order _order = { _trxid, _from, _autoExop->m_autoExNum * PRICEPRECISION, u256(0), u256(0), _timeStamp, ex::order_type::buy, ex::order_token_type::FUEL, ex::order_buy_type::all_price};
            _result =  _exdbState.insert_operation(_order);
        }
        catch(const std::exception& e)
        {
            BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("allprice buy Cookie error!")));
        }

        u256 _cookie = 0;
        for(auto _val : _result)
        {
            _cookie += _val.amount;
        }        

        if(_cookie < _baseGas)
        {
            BOOST_THROW_EXCEPTION(transferAutoExFailed() << errinfo_comment(std::string("Automatically redeemed cookies are insufficient to pay for the fuel cost of the transaction")));
        }
    }
}

bool dev::brc::BRCTranscation::findAddress(std::map<Address, u256> const& _voteData, std::vector<dev::brc::PollData> const& _pollData)
{
    bool _status = false;
    for(auto _voteDataIt : _voteData)
    { 
        for(uint32_t i = 0; i < 7 && i < _pollData.size(); i++)
        {
            if(_voteDataIt.first == _pollData[i].m_addr)
            {
                _status = true;
            }
        }
    }
    return  _status;
}

bool dev::brc::BRCTranscation::isMainNode(const std::map<Address, u256> &_voteData,  const std::vector<dev::brc::PollData> &_pollData)
{
    bool _status = false;
    for(auto voteIt : _voteData)
    {
        for(uint32_t i = 0; i < _pollData.size(); i++)
        {
            if(voteIt.first == _pollData[i].m_addr)
            {
                _status = true;
            }
        }
    }
    return _status;
}

bool dev::brc::BRCTranscation::isMainNode(const std::vector<dev::brc::PollData> &_voteData, const std::vector<dev::brc::PollData> &_pollData)
{
    bool _status = false;
    for(auto _voteIt : _voteData)
    {
        for(auto _pollIt : _pollData)
        {
            if(_voteIt.m_addr == _pollIt.m_addr)
            {
                _status = true;
            }
        }
    }
    return _status;
}