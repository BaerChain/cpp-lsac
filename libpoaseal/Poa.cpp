/*
    Poa.cpp
*/

#include "Poa.h"

#include <libethcore/ChainOperationParams.h>
#include <libethcore/CommonJS.h>
#include <libethereum/Interface.h>
using namespace std;
using namespace dev;
using namespace eth;

void dev::eth::Poa::init()
{
    ETH_REGISTER_SEAL_ENGINE(Poa);
}

void dev::eth::Poa::initEnv(std::weak_ptr<PoaHostCapability> _host)
{
	m_host = _host; 
	m_lastChange_valitor_time = 0;
	Worker::startWorking();
}

bool dev::eth::Poa::updateValitor(Address const & _address, bool _flag, int64_t _time)
{
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
		std::vector<Address>::iterator iter = m_poaValidatorAccount.begin();
		for(; iter != m_poaValidatorAccount.end(); ++iter)
		{
			if(address == *iter)
			{
				m_poaValidatorAccount.erase(iter);
				set_valitor_change_time(_time);
				sendAllUpdateValitor(address, _flag);
				return true;
			}
		}
	}
	return true;
}

void dev::eth::Poa::sendAllUpdateValitor(Address const & _address, bool _flag)
{
	PoaMsg pmsg;
	pmsg.address.push_back(Address(_address));
	pmsg.curr_valitor_index = m_poaValidatorAccount.size();
	pmsg.v_type = _flag ? VATITOR_ADD : VATITOR_DEL;
	pmsg.change_time = m_lastChange_valitor_time;

	RLPStream _s;
	pmsg.streamRLPFields(_s);
    //通知所有
	brocastMsg(PoaValitorData, _s); 
}

void dev::eth::Poa::onPoaMsg(NodeID _nodeid, unsigned _id, RLP const & _r)
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

void dev::eth::Poa::requestStatus(NodeID const & _nodeID, u256 const & /*_peerCapabilityVersion*/)
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
void dev::eth::Poa::brocastMsg(PoapPacketType _type, RLPStream & _msg_s)
{
	// 广播发送
	auto h = m_host.lock();
	h->hostFace()->foreachPeer(h->name(), 
		[&](NodeID const& _nodeId){
		sealAndSend(_nodeId, _type, _msg_s);
		cdebug << "brocastMsg ... NodeId:"<<_nodeId <<"type"<< _type;
		return true;
	    });

}
void dev::eth::Poa::sealAndSend(NodeID const & _nodeid, PoapPacketType _type, RLPStream const& _msg_s)
{
    // 封装发送
	RLPStream msg_ts;
	auto h = m_host.lock(); //weak_ptr 需要 lock（)解锁为 share_ptr才能调用
	h->hostFace()->prep(_nodeid, name(), msg_ts, _type, 1).append(_msg_s.out()); // 封装消息ID，等等
	h->hostFace()->sealAndSend(_nodeid, msg_ts);
	cdebug << " Poa Send[" << _type << "] to " << _nodeid;
}

void dev::eth::Poa::updateValitor(PoaMsg const & _poadata)
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

void dev::eth::Poa::generateSeal(BlockHeader const& _bi)
{
    cdebug << "Poa: into generateSeal";
    BlockHeader header(_bi);
    header.setSeal(NonceField, h64{0});
    header.setSeal(MixHashField, h256{0});
    RLPStream ret;
    header.streamRLP(ret);
    if (m_onSealGenerated)
        m_onSealGenerated(ret.out());
}

bool dev::eth::Poa::shouldSeal(Interface*)
{
    //TODO 实现判断算法执行前逻辑
    return true;
}

void dev::eth::Poa::workLoop()
{
	cdebug << "Poa::workLoop()";
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
			cdebug << " get mesg: PoaStatusPacket||"<< ret.second.packet_id << ret.second.node_id;
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
				cdebug << " get mesg: PoaRequestValitor||" << ret.second.packet_id << ret.second.node_id;
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
				cdebug << " get mesg: PoaValitorData||" << ret.second.packet_id << ret.second.node_id;
				PoaMsg retMsg;
				retMsg.populate(RLP(ret.second.data));
                //检查自己本地数据状态
				if(retMsg.last_time != m_lastChange_valitor_time)
				{
                    //不一致需要更新
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


