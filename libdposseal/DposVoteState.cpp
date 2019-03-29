#include "DposVoteState.h"
#include <libbrccore/TransactionBase.h>

using namespace dev::brc;

/**********************DposState start****************************/
void dev::bacd::DposVoteState::excuteTransation(TransactionBase const & _t, BlockHeader const & _h)
{
	LOG(m_logger) << BrcYellow << " excuteTransation ......................." << BrcReset;
	if(_t.data().empty())
		return;
	RLP rlp(_t.data());
	Json::Reader data_reader;
	Json::Value _json;
	if(rlp.itemCount() == 1)
	{
		if(!data_reader.parse(rlp[0].toString(), _json))
			return;
	}
	else
		return;
	if(!_json.isMember("type") || !_json.isMember("from") || !_json.isMember("to") )
		return;

	u256  _type = u256(_json["type"].asString());
	if(_type < 3 || _type >6)
		return;

	DposTransaTionResult t_ret;
    if(_type == 3 || _type ==4)
	{
		if(!_json.isMember("tickets"))
			return;
		u256 ticks = u256(_json["tickets"].asString());
		t_ret.m_vote = (size_t)ticks;
	}

	t_ret.m_form = Address(_json["from"].asString());
	t_ret.m_send_to = Address(_json["to"].asString());
    t_ret.m_type =(EDposDataType)((size_t)_type);
	t_ret.m_effect = e_used;

    t_ret.m_hash = _t.sha3();
    t_ret.m_epoch = _h.timestamp() / m_config.epochInterval; //time(NULL) / epochInterval;
	t_ret.m_block_hight = _h.number();

	//m_cacheTransation.push_back(t_ret);
    m_transations.push_back(t_ret);
    m_onResult.push_back(OnDealTransationResult(EDPosResult::e_Add, t_ret));
	setUpdateTime(utcTimeMilliSec());
    cdebug <<BrcYellow "excute transation ret:" << t_ret << BrcReset;
}

void dev::bacd::DposVoteState::excuteTransation(bytes const & _t, BlockHeader const & _h)
{
    TransactionBase _tb = TransactionBase(_t, CheckTransaction::Cheap);
    excuteTransation(_tb, _h);
}

void dev::bacd::DposVoteState::verifyVoteTransation(BlockHeader const & _h, h256s const & _t_hashs)
{
    if(m_transations.empty() || _t_hashs.empty())
        return;
    /* for (auto val : _t_hashs)
     {
         LOG(m_logger) << BrcYellow << _h.number() - m_config.verifyVoteNum << " Block transations hash" << val << BrcYellow;
     }*/
    m_onResult.clear();
    //size_t curr_epoch = _h.timestamp() / m_config.epochInterval;
    std::vector<DposTransaTionResult>::iterator iter = m_transations.begin();
    for(; iter != m_transations.end();)
    {
        //LOG(m_logger) << BrcYellow "m_transations epoch:" << iter->m_epoch << "|cuur_epoch :" << curr_epoch << BrcYellow;
        LOG(m_logger) << BrcYellow "result:" << *iter << BrcYellow;
        if(iter->m_effect == e_timeOut)
		{
			m_onResult.push_back(OnDealTransationResult(EDPosResult::e_Dell, *iter));
			iter = m_transations.erase(iter);
		}
        else
        {
            //在第verifyVoteNum个块之后执行
            if(iter->m_block_hight <= (int64_t)(_h.number() - m_config.verifyVoteNum))
            {
                //区块数达到要求   执行交易
                DposTransaTionResult tranRet = *iter;
                auto ret = std::find(_t_hashs.begin(), _t_hashs.end(),tranRet.m_hash);
                if(ret != _t_hashs.end())
                {
                    LOG(m_logger) << "will deal Transation hash:" << tranRet.m_hash;
                    iter = m_transations.erase(iter);
                    m_onResult.push_back(OnDealTransationResult(EDPosResult::e_Dell, tranRet));
                }
                else
                {
                    LOG(m_logger) << "cant't find Transation hash:" << iter->m_hash;
                    ++iter;
                }
            }
            else
            {
                LOG(m_logger) << "verify BlockNum not enough Transation hash:" << iter->m_hash;
                ++iter;
            }
        }
    }
}

void dev::bacd::DposVoteState::updateVoteTransation(OnDealTransationResult const & ret)
{
    switch(ret.m_ret_type)
    {
    case e_Add:
    {
        
        DposTransaTionResult result(static_cast<DposTransaTionResult>(ret));
        insertDposTransaTionResult(result);
        break;
    }
    case  e_Dell:
    {

        std::vector<DposTransaTionResult>::iterator iter = m_transations.begin();
        for(; iter != m_transations.end(); ++iter)
        {
            if(iter->m_hash == ret.m_hash)
            {
                m_transations.erase(iter);
                return;
            }
        }
        break;
    }
    default:
    break;
    }
}

bool dev::bacd::DposVoteState::insertDposTransaTionResult(DposTransaTionResult const& _d)
{
    for(auto val : m_transations)
    {
        if(_d.m_hash == val.m_hash)
            return false;
    }
    m_transations.push_back(_d);
    return true;
}

/**********************DposState end****************************/