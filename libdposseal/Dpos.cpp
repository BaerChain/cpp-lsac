#include "Dpos.h"

dev::bacd::Dpos::Dpos()
{
	m_varlitor_vote.clear();
	m_vote_varlitor.clear();
	m_curr_valitor.clear();
	m_curr_valitors.clear();
	m_canditate.clear();
	m_prveslot = 0;
	m_currslot = 0;
}

void dev::bacd::Dpos::generateSeal(BlockHeader const & _bi)
{
	BlockHeader header(_bi);
	header.setSeal(NonceField, h64{ 0 });
	header.setSeal(MixHashField, h256{ 0 });
	RLPStream ret;
	header.streamRLP(ret);
	if(m_onSealGenerated)
		m_onSealGenerated(ret.out());
}

void dev::bacd::Dpos::workLoop()
{
    
}

void dev::bacd::Dpos::init()
{
	ETH_REGISTER_SEAL_ENGINE(Dpos);
}

void dev::bacd::Dpos::initEnv(std::weak_ptr<DposHostcapality> _host)
{
	m_host = _host;
}

void dev::bacd::Dpos::onPoaMsg(NodeID _nodeid, unsigned _id, RLP const & _r)
{
	if(_id < DposPacketCount && _id >= DposStatuspacket)
	{
		cdebug << "onRaftMsg: id=" << _id << ",from=" << _nodeid;
		m_msg_queue.push(DposMsgPacket(_nodeid, _id, _r[0].data()));

	}
	else
	{
		cwarn << "Recv an illegal msg, id=" << _id << "  and the max_id:" << DposPacketCount - 1;
	}
}

void dev::bacd::Dpos::requestStatus(NodeID const & /*_nodeID*/, u256 const & /*_peerCapabilityVersion*/)
{

}

void dev::bacd::Dpos::brocastMsg(DposPacketType _type, RLPStream & _msg_s)
{
	// 广播发送  将 sealAndSend() 回调使用
	auto h = m_host.lock();
	h->hostFace()->foreachPeer(h->name(),
		[&](NodeID const& _nodeId){
		sealAndSend(_nodeId, _type, _msg_s);
		cdebug << "brocastMsg ... NodeId:" << _nodeId << "type" << _type;
		return true;
	});
}

void dev::bacd::Dpos::sealAndSend(NodeID const & _nodeid, DposPacketType _type, RLPStream const & _msg_s)
{
	// 封装发送
	RLPStream msg_ts; //网络层数据包 ，会将业务逻辑追加包封装到最后
	auto h = m_host.lock(); //weak_ptr 需要 lock（)解锁为 share_ptr才能调用
	h->hostFace()->prep(_nodeid, name(), msg_ts, _type, 1).append(_msg_s.out()); // 封装消息ID，等等
	h->hostFace()->sealAndSend(_nodeid, msg_ts);
	cdebug << " Poa Send[" << _type << "] to " << _nodeid;
}

void dev::bacd::Dpos::updateDposData(EDposDataType _type, Address const & _addr_source, Address const & _addr_target)
{
    //dpos 选举人/验证人 更新
	Debug << "updateDposData|" << "type:" << _type << " _addr_source:" << _addr_source << " _addr_target" << _addr_target;
	switch(_type)
	{
	case dev::bacd::e_loginCandidate:
		break;
	case dev::bacd::e_logoutCandidate:
		break;
	case dev::bacd::e_delegate:
		break;
	case dev::bacd::e_unDelegate:
		break;
	case dev::bacd::e_max:
		break;
	default:
		break;
	}
}

bool dev::bacd::Dpos::isBolckSeal(uint64_t _last_time, uint64_t _now)
{
	Debug << "isBolckSeal: _last_time:" << _last_time << "||_now:" << _now;
	if(!CheckValidator(_now))
		return false;
	if(_last_time == 0)
		return true;
	if(tryElect(_last_time, _now))
		return false; //进入下一轮
	return true;
}

bool dev::bacd::Dpos::checkDeadline(uint64_t _last_block_time, uint64_t _now)
{  
	if(m_curr_valitors.empty())
		return false;
    if(_last_block_time <=0)
	{
	    //如果为0 可能是创世区块
		return false;
	}
	//得到每次出块的整数时间刻度，比较上次，现在和下次
	// int64((now+blockInterval-1)/blockInterval) * blockInterval
	int next_slot = (_now + blockInterval - 1) / blockInterval * blockInterval;
    //nt64((now-1)/blockInterval) * blockInterval
	int prve_slot = (_now - 1) / blockInterval * blockInterval;
	int last_slot = (_last_block_time - 1) / blockInterval * blockInterval;

    if(last_slot >= next_slot)
	{
		cwarn << "waring :" << "last_slot:"<<last_slot<<" >= next_slot:"<<next_slot;
		return false;
	}
	if(last_slot == prve_slot || (next_slot - _now) <= 1)
		return true;
	return false;
}

bool dev::bacd::Dpos::CheckValidator(uint64_t _now)
{
	if(m_curr_valitors.empty())
		return false;

	int offet = _now % epochInterval;       // 当前轮 进入了多时间
	if(offet % blockInterval != 0)
		return false;   //此时间在出块最短时间 返回

	offet /= blockInterval;
	offet %= m_curr_valitors.size();
	Address const& curr_valitor = m_curr_valitors[offet]; //得到当验证人
	if(m_curr_valitor == curr_valitor)
		return true;
	return false;
}

bool dev::bacd::Dpos::tryElect(uint64_t _last_time, uint64_t _now)
{
    //这里 验证人已经通过 尝试统计投票进入下一轮 
    //失败 则继续出块
	unsigned int prveslot = _last_time / epochInterval; //上一个块的周期
	unsigned int currslot = _now / epochInterval;   //当前即将出块的周期
	cdebug << "prveslot:" prveslot << " |currslot" << currslot;
    if(prveslot == currslot)
	{
	    //处于相同周期 出块
		return false;
	}
    // 触发提出不合格验证人
	kickoutValidator(prveslot);
    //统计投票
	countVotes();
    //打乱验证人顺序
	disorganizeVotes();
    //广播同步新一轮的验证人
    //brocastMsg(DposDataPacket, )
	return true;
}

void dev::bacd::Dpos::kickoutValidator(unsigned int _prveslot)
{
    //剔除不合格
	cdebug << "kickoutValidator:" << _prveslot;
    //上一个轮创世 不踢
	if(m_prveslot == 0 && m_canditate.size() <=maxValitorNum)
		return;
	unsigned int kickout_num = m_canditate.size() - maxValitorNum; // 提出验证人人数


}
