/*
    Poa.cpp
*/

#include "Poa.h"

#include <libbrccore/ChainOperationParams.h>
#include <libbrccore/CommonJS.h>
#include <libbrcdchain/Interface.h>
using namespace std;
using namespace dev;
using namespace brc;

void dev::brc::Poa::init()
{
    BRC_REGISTER_SEAL_ENGINE(Poa);
}

void dev::brc::Poa::initEnv(std::weak_ptr<PoaHostCapability> _host)
{
	m_host = _host; 
	m_lastChange_valitor_time = 0;
}
/*
    验证人更新 
	_address： 更新地址 
	_flag ： true 添加
	_time ：更新时间
*/
bool dev::brc::Poa::updateValitor(Address const & _address, bool _flag, int64_t _time)
{
	//cdebug << "updateValitor: _address:" << _address << "|_flag:" << _flag << "|time:"<<_time;
	Address address(_address);
	Guard l(m_mutex);
	if(_flag)
	{
        //添加验证人
		for(auto val : m_poaValidatorAccount)
		{
			if(val == address)
				return false;
		}
		m_poaValidatorAccount.push_back(address);
		set_valitor_change_time(_time); 
		sendAllUpdateValitor(address, _flag);
	}
	else
	{
        //删除
		unsigned int index = 0;
		std::vector<Address>::iterator iter = m_poaValidatorAccount.begin();
		for(; iter != m_poaValidatorAccount.end(); ++iter)
		{
			if(address == *iter)
			{
				//添加删除缓存
				DelPoaValitor delva;
				delva.m_del_address = Address(*iter);
				unsigned int next_index = (index + 1) >= m_poaValidatorAccount.size() ? 0 : index + 1; //考虑最后一个元素
				delva.m_next_address = Address(m_poaValidatorAccount[next_index]);
				m_del_poaValitors.push_back(delva);
                //删除 广播通知
				m_poaValidatorAccount.erase(iter);
				set_valitor_change_time(_time);
				sendAllUpdateValitor(address, _flag);
				return true;
			}
			index++;
		}
	}
	return true;
}

void dev::brc::Poa::initPoaValidatorAccounts(std::vector<Address> const & _addresses)
{
	if(_addresses.empty())
		return;
	m_poaValidatorAccount.clear();
	m_poaValidatorAccount.assign(_addresses.begin(), _addresses.end());

	m_del_poaValitors.clear();
}

bool dev::brc::Poa::isvalidators(Address const & _our_address, Address const & _currBlock_address)
{
	if(m_poaValidatorAccount.empty())
	{
		cwarn << "POA:"
			<< "don't have validators !";
		return false;
	}
	//暂时验证 指定出块的验证人
	int curr_site = 1;  //定位验证人
	for(auto const& val : m_poaValidatorAccount)
	{
		if(val == _our_address)
			break;
		curr_site++;
	}
	int validators_size = m_poaValidatorAccount.size();
	if(curr_site > validators_size)
	{
		cdebug << "the autor not is Validator";
		return false;
	}
	// 验证 验证
	// 判断addressCurr的下一个是否为自己, 也就是 我自己位置上一个是否为addressCurr
	int last_index = (curr_site - 1) == 0 ? validators_size - 1 : curr_site - 2;

	if(m_poaValidatorAccount[last_index] == _currBlock_address )
		return true;
	else
	{
        //存在 当前块的验证人刚刚被删除 在m_poaValidatorAccount 中不存在，应该在 m_del_poaValitors 中
		cdebug << " have delVator ...";
		auto find_ret = find(m_del_poaValitors.begin(), m_del_poaValitors.end(), _currBlock_address);
		bool ret = false;
		if(find_ret != m_del_poaValitors.end())
		{
            //当前块的验证人已经被撤销 检验它下一个验证人是否为our
			if((*find_ret).m_next_address == _our_address)
				ret = true;
			//已经删除的验证人缓存 用一次就可以删除
			m_del_poaValitors.erase(find_ret);
		}
		if(ret)
			return true;
	}
	cdebug << " this turns not is mine: autor[" << _our_address << "]"
		<< "site:" << curr_site - 1;
	return false;
}

void dev::brc::Poa::sendAllUpdateValitor(Address const & _address, bool _flag)
{
	PoaMsg pmsg;
	pmsg.address.push_back(Address(_address));
	pmsg.curr_valitor_index = m_poaValidatorAccount.size();
	pmsg.v_type = _flag ? VATITOR_ADD : VATITOR_DEL;
	pmsg.last_time = m_lastChange_valitor_time;

	RLPStream _s;
	pmsg.streamRLPFields(_s);
    //通知所有
	brocastMsg(PoaValitorData, _s); 
}

void dev::brc::Poa::onPoaMsg(NodeID _nodeid, unsigned _id, RLP const & _r)
{
	if(_id < PoaPacketCount)
	{
		cdebug << "onRaftMsg: id=" << _id << ",from=" << _nodeid;
		m_msg_queue.push(PoaMsgPacket(_nodeid, _id, _r[0].data()));
        
	}
	else
	{
		cwarn << "Recv an illegal msg, id=" << _id << "  and the max_id:"<< PoaPacketCount -1;
	}
}

void dev::brc::Poa::requestStatus(NodeID const & _nodeID, u256 const & /*_peerCapabilityVersion*/)
{
	//
	cdebug << "send status to:" << _nodeID;
	PoaMgBase resq;
	resq.total_valitor_size = m_poaValidatorAccount.size();
	resq.last_time = m_lastChange_valitor_time;

	RLPStream resp_ts;
	resq.streamRLPFields(resp_ts);

	sealAndSend(_nodeID, PoaStatusPacket, resp_ts);
}
void dev::brc::Poa::brocastMsg(PoapPacketType _type, RLPStream & _msg_s)
{
	// 广播发送  将 sealAndSend() 回调使用
	auto h = m_host.lock();
	h->hostFace()->foreachPeer(h->name(), 
		[&](NodeID const& _nodeId){
		sealAndSend(_nodeId, _type, _msg_s);
		cdebug << "brocastMsg ... NodeId:"<<_nodeId <<"type"<< _type;
		return true;
	    });

}
void dev::brc::Poa::sealAndSend(NodeID const & _nodeid, PoapPacketType _type, RLPStream const& _msg_s)
{
    // 封装发送
	RLPStream msg_ts;
	auto h = m_host.lock(); //weak_ptr 需要 lock（)解锁为 share_ptr才能调用
	h->hostFace()->prep(_nodeid, name(), msg_ts, _type, 1).append(_msg_s.out()); // 封装消息ID，等等
	h->hostFace()->sealAndSend(_nodeid, msg_ts);
	cdebug << " Poa Send[" << _type << "] to " << _nodeid;
}

void dev::brc::Poa::updateValitor(PoaMsg const & _poadata)

{
	if(_poadata.address.empty())
	{
		cwarn << "updateValitor VATITOR_ADD the address is empty";
		return;
	}
	m_lastChange_valitor_time = _poadata.last_time;
	switch(_poadata.v_type)
	{
        case VATITOR_ADD:
		{
			updateValitor(_poadata.address[0], true, m_lastChange_valitor_time);
            break;
		}
		case VATITOR_DEL:
		{
			updateValitor(_poadata.address[0], false, m_lastChange_valitor_time);
            break;
		}
		case VATITOR_FULL:
		{
			Guard l(m_mutex);
			m_poaValidatorAccount.clear();
			for(auto val : _poadata.address)
			{
				m_poaValidatorAccount.push_back(Address(val));
			}
            break;
		}
	default:
		break;
	}
}

void dev::brc::Poa::generateSeal(BlockHeader const& _bi)
{
    BlockHeader header(_bi);
    header.setSeal(NonceField, h64{0});
    header.setSeal(MixHashField, h256{0});
    RLPStream ret;
    header.streamRLP(ret);
    if (m_onSealGenerated)
        m_onSealGenerated(ret.out());
}

bool dev::brc::Poa::shouldSeal(Interface*)
{
    //TODO 实现判断算法执行前逻辑
    return true;
}

void dev::brc::Poa::workLoop()
{
	while(isWorking())
	{
		std::pair<bool, PoaMsgPacket> ret = m_msg_queue.tryPop(5);
		if(!ret.first)
		{
			//cdebug << "Poa::workLoop()333";
			continue;
		}
		else
		{
			cdebug << " get mesg: ||"<< ret.second.packet_id << ret.second.node_id;
			switch(ret.second.packet_id)
			{
			case PoaStatusPacket:
			{
				PoaMgBase msg;
				msg.populate(RLP(ret.second.data));
                //判断poa验证人状态
				if(m_lastChange_valitor_time < msg.last_time)
				{
                    //本地valitor 不是最新 请求最新数据
					cdebug << "this valitor is older and to  request new data...";
					sealAndSend(ret.second.node_id, PoaRequestValitor, RLPStream());
				}
				else 
				{
					cdebug << "our varlitor is new ...";
				}
                break;
			}
			case PoaRequestValitor:
			{
                //收到请求valitor 数据
				PoaMsg p_msg;
				p_msg.address.assign(m_poaValidatorAccount.begin(), m_poaValidatorAccount.end());
				p_msg.v_type = VATITOR_FULL;
				p_msg.last_time = m_lastChange_valitor_time;

				RLPStream _ps;
				p_msg.streamRLPFields(_ps);

				sealAndSend(ret.second.node_id, PoaValitorData, _ps);
                break;
			}
			case PoaValitorData:
			{
				PoaMsg retMsg;
				retMsg.populate(RLP(ret.second.data));
                //检查自己本地数据状态
				retMsg.printLog();
				if(retMsg.last_time != m_lastChange_valitor_time)
				{
                    //不一致需要更新
					cdebug << "updateValitor ......";
					updateValitor(retMsg);
				}
				break;
			}
			default:
				break;
			}
		}
	}
}


