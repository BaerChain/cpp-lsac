#include "Dpos.h"
#include <libethcore/TransactionBase.h>
#include <cstdlib>

dev::bacd::Dpos::Dpos()
{
    m_dpos_cleint =nullptr;
    m_last_dposDataMsg_time = 0;
    m_last_create_block_time = 0;
}

dev::bacd::Dpos::~Dpos()
{
    m_dpos_cleint = nullptr;
    stopWorking();
}

void dev::bacd::Dpos::generateSeal(BlockHeader const & _bi)
{
    BlockHeader header(_bi);
    header.setSeal(NonceField, h64{ 0 });
    header.setSeal(MixHashField, h256{ 0 });
    RLPStream ret;     
    header.streamRLP(ret);
    if(m_onSealGenerated)
    {
        m_onSealGenerated(ret.out());
    }
    // 同步dpos数据
    syncVoteData();
}

void dev::bacd::Dpos::initGenesieVarlitors(ChainParams const & m_params)
{
    m_genesis_varlitor.assign(m_params.poaValidatorAccount.begin(), m_params.poaValidatorAccount.end());
}

void dev::bacd::Dpos::init()
{
    ETH_REGISTER_SEAL_ENGINE(Dpos);
}

void dev::bacd::Dpos::initEnv(std::weak_ptr<DposHostcapality> _host)
{
    m_host = _host;
}

void dev::bacd::Dpos::workLoop()
{
    while(isWorking())
    {
        std::pair<bool, DposMsgPacket> ret = m_msg_queue.tryPop(5);
        if(!ret.first)
        {
            continue;
        }
        cdebug <<EthBlue " get mesg: ||" << ret.second.packet_id << ret.second.node_id << EthBlue;
        switch(ret.second.packet_id)
        {
        case DposStatuspacket:
        {
            DposDataMsg dataMsg;
            dataMsg.populate(RLP(ret.second.data));
            LOG(m_logger) <<EthYellow "recved DposStatuspacket msg : " << dataMsg << EthYellow;
            if(!isVarlitor(dataMsg.m_addr))
            {
                LOG(m_logger) << "node_id:" << ret.second.node_id << "|addrss:" << dataMsg.m_addr << " is not varlitor";
                return;
            }
            if(dataMsg.m_now == m_last_dposDataMsg_time )
                return;
            else if(dataMsg.m_now > m_last_dposDataMsg_time)
            {
                for(auto val : dataMsg.m_transation_ret)
                    updateVoteData(val);
                m_last_dposDataMsg_time = dataMsg.m_now;
            }
            else
            {
                requestStatus(ret.second.node_id, 0);
            }

            break;
        }
        case DposDataPacket:
        {
            DposDataMsg msg;
            msg.populate(RLP(ret.second.data));
            LOG(m_logger) << EthYellow "recved msg : " << msg << EthYellow;
            if(!isVarlitor(m_dpos_cleint->author()))
                return;
            for(auto val: msg.m_transation_ret)
            {
                updateVoteData(val);
            }
            if(msg.m_now != m_last_dposDataMsg_time)
            {
                m_last_dposDataMsg_time = msg.m_now;
                RLPStream _s;
                msg.streamRLPFields(_s);
                brocastMsg(DposDataPacket, _s);
            }
            break;
        }
        default:
            break;
        }
    }
}

void dev::bacd::Dpos::requestStatus(NodeID const & _nodeID, u256 const & /*_peerCapabilityVersion*/)
{
    LOG(m_logger) << "*******Dpos: requestStatus*********";
    if(!m_dpos_cleint->dposVoteState()->isVerifyVoteTransation())
        return;
    LOG(m_logger) << "send status to:" << _nodeID;
    DposDataMsg msg;
    msg.m_addr = m_dpos_cleint->author();
    msg.m_now = m_last_dposDataMsg_time;
    std::vector<DposTransaTionResult> ret = m_dpos_cleint->dposVoteState()->getTransationResult();
    for(auto val : ret)
    {
        OnDealTransationResult result(EDPosResult::e_Add, false, val);
        msg.m_transation_ret.push_back(result);
    }
    LOG(m_logger) << msg;
    RLPStream _s;
    msg.streamRLPFields(_s);
    sealAndSend(_nodeID, DposStatuspacket, _s);
}

void dev::bacd::Dpos::onDposMsg(NodeID _nodeid, unsigned _id, RLP const & _r)
{
    if(_id < DposPacketCount && _id >= DposStatuspacket)
    {
        cdebug << EthRed "onRaftMsg: id=" << _id << ",from=" << _nodeid << EthRed;
        m_msg_queue.push(DposMsgPacket(_nodeid, _id, _r[0].data()));
    }
    else
    {
        cwarn << "Recv an illegal msg, id=" << _id << "  and the max_id:" << DposPacketCount - 1;
    }
}

void dev::bacd::Dpos::sealAndSend(NodeID const & _nodeid, DposPacketType _type, RLPStream const & _msg_s)
{
    // 封装发送
    RLPStream msg_ts;
    auto h = m_host.lock(); //weak_ptr 需要 lock（)解锁为 share_ptr才能调用
    h->hostFace()->prep(_nodeid, name(), msg_ts, _type, 1).append(_msg_s.out()); // 封装消息ID，等等
    h->hostFace()->sealAndSend(_nodeid, msg_ts);
    cdebug << " Dpos Send[" << _type << "] to " << _nodeid;
}

bool dev::bacd::Dpos::isBolckSeal(uint64_t _now)
{
    if(!CheckValidator(_now))
    {
        return false;
    } 
    //同步本地dpos
    initDataByCurrBlock();

    printDposData(m_dpos_context);
    return true;
}

bool dev::bacd::Dpos::checkDeadline(uint64_t _now)
{  
    //LOG(m_logger) <<"the curr and genesis hash:"<< m_dpos_cleint->getCurrBlockhash() << "||" << m_dpos_cleint->getGenesisHash();
    if(m_dpos_cleint->getCurrBlockhash() == m_dpos_cleint->getGenesisHash())
    {
        //parent 创世区块 不进行时间验证
        //验证人为指定验证人
        //initVarlitorByGenesis();
        return true;
    }
    const BlockHeader _h = m_dpos_cleint->getCurrHeader();
    int64_t _last_time = _h.timestamp();
    if(_last_time <=0)
    {
        cwarn << " error! the block time is 0!";
        return false;
    }
    // 由于轮询时间过快，10ms 类多次进入
    int64_t comp_time = _now - 10;
    if(_last_time >= comp_time )
    {
        LOG(m_warnlog) << "the time is error ..";
        return false;
    }
    //得到每次出块的整数时间刻度，比较上次，现在和下次
    //系统时间算出的下一个出块时间点
	uint64_t next_slot = (_now + blockInterval - 1) / blockInterval * blockInterval;
    //当前块算出的上一个出块时间点
	uint64_t last_slot = (_last_time - 1) / blockInterval * blockInterval;
    //当前块算出即将出块时间点
	uint64_t curr_slot = last_slot + blockInterval;

	/*cdebug << "_now:      " << _now;
	cdebug << "_last_time:" << _last_time;
	cdebug << "next_slot: " << next_slot;
	cdebug << "last_slot: " << last_slot;
	cdebug << "curr_slot: " << curr_slot;*/

    if(curr_slot <= _now || (next_slot - _now) <= 1)
    {
        return true;
    }
    LOG(m_logger) << EthYellow"the slot time have some error! _now:"<< _now<< EthReset;
    return false;
}

void dev::bacd::Dpos::initVarlitorByGenesis()
{
    m_dpos_context.curr_varlitor.assign(m_genesis_varlitor.begin(), m_genesis_varlitor.end());
}

bool dev::bacd::Dpos::CheckValidator(uint64_t _now)
{
    cdebug << " into CheckValidator...";
    const BlockHeader _h = m_dpos_cleint->getCurrHeader();
    std::vector<Address> const& _currvar= _h.dposContext().curr_varlitor;
    std::vector<Address> const& _gennesis_var = m_dpos_cleint->getGenesisHeader().dposContext().curr_varlitor;
    std::vector<Address> varlitors;
    varlitors.clear();
    varlitors.assign(_currvar.begin(), _currvar.end());
    if(_currvar.empty())
    {
        LOG(m_warnlog) << "the m_dpos_context's curr_varlitor is empty and try to use genesis varlitor...";
        if(_gennesis_var.empty())
        {
            LOG(m_logger) << " the genesis varlitor is empty, can't to create block ...";
            return false;
        }
        varlitors.clear();
        varlitors.assign(_gennesis_var.begin(), _gennesis_var.end());
    }
	LOG(m_logger) << EthYellow << "_now:" << _now;
    uint64_t offet = _now % epochInterval;       // 当前轮 进入了多时间
    //if(offet % blockInterval != 0)
    //{
    //    LOG(m_logger) << "this time not is create block time!";
    //    return false;   //此时间在出块最短时间 返回
    //}
	LOG(m_logger) << "offet = _now % epochInterval:" << offet;
    offet /= varlitorInterval;
	LOG(m_logger) << "offet /= varlitorInterval:" << offet;
    offet %= varlitors.size();
	LOG(m_logger) << "offet %= varlitors.size():" << offet;
    Address const& curr_valitor = varlitors[offet]; //得到当验证人
    for (auto val : varlitors)
    {
        cdebug << val << "offet:"<< offet;
    }
    if(m_dpos_cleint->author() == curr_valitor)
    {
        m_last_create_block_time = _now;
        return true;
    }
    return false;
}

void dev::bacd::Dpos::tryElect(uint64_t _now)
{
    //这里 验证人已经通过 尝试统计投票 
    //尝试后继续出块 则继续出块
    const BlockHeader _h = m_dpos_cleint->getCurrHeader();
    uint64_t _last_time = _h.timestamp();
    unsigned int prveslot = _last_time / epochInterval; //上一个块的周期
    unsigned int currslot = _now / epochInterval;   //当前即将出块的周期
    //cdebug << EthYellow"？？？？prveslot:"<< prveslot << " |currslot" << currslot;
	//cdebug << EthYellow"parent hash:" << m_dpos_cleint->getCurrBlockhash();
	cdebug << EthYellow"_last_time: " << _last_time << "|now:"<<_now;
    if(prveslot == currslot)
    {
        //处于相同周期 出块
        return ;
    }
   // LOG(m_logger) << "start new epoch...";

    // 触发提出不合格验证人
    kickoutValidator();
    //统计投票
    countVotes();
    //打乱验证人顺序
    disorganizeVotes();
    
    LOG(m_logger) <<EthYellow "******Come to new epoch, prevEpoch:"<< prveslot << "nextEpoch:" << currslot<< EthYellow;
}

void dev::bacd::Dpos::kickoutValidator()
{
    //剔除不合格 当下一轮验证人+ 创世配置验证人 > maxValitorNum 触发
    BlockHeader const& genesisHeader = m_dpos_cleint->getGenesisHeader();
    DposContext const& genesis_dpos_data = genesisHeader.dposContext();
    int varlitor_num = maxValitorNum - genesis_dpos_data.curr_varlitor.size();
    if(varlitor_num <=0)
    {
        LOG(m_logger) << "next slot varlitor num <" << maxValitorNum << " and not to kickout...";
        return;
    }
    //踢人逻辑

}

void dev::bacd::Dpos::countVotes()
{
    //统计投票
    std::vector<DposVarlitorVote> varlitor_vote;
    for (auto var_vote : m_dpos_context.varlitors_votes.m_varlitor_voter)
    {
        if(var_vote.second.size() ==0)
            continue;
        //检查是否为候选人
        if(m_dpos_context.canlidates.find(var_vote.first) == m_dpos_context.canlidates.end())
            continue;
        //检查投票人是否为当前验证人
        size_t num = 0;
        for(auto val : var_vote.second)
        {
            auto voter = m_dpos_context.vote_varlitor.find(val);
            if(voter == m_dpos_context.vote_varlitor.end())
                continue;
            if(voter->second == var_vote.first)
                ++num;
        }
        varlitor_vote.push_back(DposVarlitorVote(var_vote.first, num));
    }
    if(varlitor_vote.size() > 1)
    {
        std::sort(varlitor_vote.begin(), varlitor_vote.end());
    }

    //insert to curr_varlitor
    m_dpos_context.curr_varlitor.clear();
    DposContext const& genesis_dpos_data =m_dpos_cleint->getGenesisHeader().dposContext();
    size_t varlitor_num = maxValitorNum - genesis_dpos_data.curr_varlitor.size();
    for (size_t i=0; i< varlitor_num; i++)
    {   if(varlitor_vote.size() > i)
            m_dpos_context.curr_varlitor.push_back(varlitor_vote[i].m_addr);
    }
    //加入创世配置验证人
    for(auto val : genesis_dpos_data.curr_varlitor)
    {
        cdebug << "genesis_dpos_data:" << val;
        m_dpos_context.curr_varlitor.push_back(Address(val));
    }
    for(auto val: m_dpos_context.curr_varlitor)
    {
        cdebug << val;
    }

    //清空相关数据
    m_dpos_context.varlitor_block_num.clear();
    m_dpos_context.vote_varlitor.clear();
	m_dpos_context.varlitors_votes.clear();
    //待考虑候选人时候清空
}

void dev::bacd::Dpos::disorganizeVotes()
{
    //随机打乱验证人

    int range = m_dpos_context.curr_varlitor.size();
    // rand()%(Y-X+1)+X [X,Y]
    //rand()%range
    //使用 parent hash 种子
    //size_t parent_hash =(size_t)m_dpos_cleint->getCurrBlockhash();
    srand(time(NULL));
    for(int i=0; i < range; i++)
    {
        int j = rand() % range;
        Address temp_addr = m_dpos_context.curr_varlitor[i];
        m_dpos_context.curr_varlitor[i] = m_dpos_context.curr_varlitor[j];
        m_dpos_context.curr_varlitor[j] = temp_addr;
    }
}

void dev::bacd::Dpos::updateVoteData(OnDealTransationResult const & _ret)
{
    m_dpos_cleint->dposVoteState()->updateVoteTransation(_ret);
}

void dev::bacd::Dpos::dealVoteDatas()
{
    LOG(m_logger) << EthRed "************deal dealVoteDatas :" << EthRed;
    std::vector<OnDealTransationResult> const& ret = m_dpos_cleint->dposVoteState()->getOnTransationResult();
    if(ret.empty())
        return;
    for(auto val : ret)
    {
        if(!val.m_is_deal_vote)
            continue;
        dealVoteData(val);
    }
}

void dev::bacd::Dpos::dealVoteData(OnDealTransationResult const& _ret)
{
    LOG(m_logger) << EthRed "************deal dpos vote :" << _ret << EthRed;
    //处理投票
    EDposDataType type = _ret.m_type;

    Address const& addr_from = _ret.m_form;
    Address const& addr_sender_to = _ret.m_send_to;
    switch(type)
    {
    case dev::bacd::e_loginCandidate:
    {
        m_dpos_context.canlidates.insert(addr_sender_to);
    }
    break;
    case dev::bacd::e_logoutCandidate:
    {
        auto ret = std::find(m_dpos_context.canlidates.begin(), m_dpos_context.canlidates.end(), addr_sender_to); //m_dpos_context.canlidates.find( _t.sender());
        if(ret != m_dpos_context.canlidates.end())
            m_dpos_context.canlidates.erase(ret);
    }
    break;
    case dev::bacd::e_delegate:
    {
        //推荐为验证人
        //删除原来的 投票人-验证人
        auto vote_ret = m_dpos_context.vote_varlitor.find(addr_from);
        if(vote_ret != m_dpos_context.vote_varlitor.end())
            m_dpos_context.vote_varlitor.erase(vote_ret);
        //删除旧的 验证人-投票人s
        m_dpos_context.varlitors_votes.del_old_varlitor(addr_sender_to, addr_from);
        //添加新的 验证人-投票人s
        m_dpos_context.varlitors_votes.add_vartor_vote(addr_sender_to, addr_from);
        //添加新的 投票人-验证人
        m_dpos_context.vote_varlitor.insert(std::make_pair(addr_from, addr_sender_to));
    }
    break;
    case dev::bacd::e_unDelegate:
    {
        auto vote_ret = m_dpos_context.vote_varlitor.find(addr_from);
        if(vote_ret != m_dpos_context.vote_varlitor.end())
            m_dpos_context.vote_varlitor.erase(vote_ret);
        //删除旧的 验证人-投票人s
        m_dpos_context.varlitors_votes.del_old_varlitor(addr_sender_to, addr_from);
    }
    break;
    case dev::bacd::e_max:
    break;
    default:
    break;
    }
    printDposData(m_dpos_context);
}

void dev::bacd::Dpos::syncVoteData()
{
    LOG(m_logger) << EthYellow " start to syncVoteData ..." << EthYellow;
    if(!m_dpos_cleint->dposVoteState()->isSyncVoteTransation())
        return;
    m_last_dposDataMsg_time = utcTimeMilliSec();
    std::vector<OnDealTransationResult> const& ret = m_dpos_cleint->dposVoteState()->getOnTransationResult();
    DposDataMsg msg;
    msg.m_addr = m_dpos_cleint->author();
    msg.m_now = m_last_dposDataMsg_time;
    msg.m_transation_ret.assign(ret.begin(), ret.end());
    LOG(m_logger) << EthYellow<< msg<< EthYellow;
    RLPStream _s;
    msg.streamRLPFields(_s);
    brocastMsg(DposDataPacket, _s);
}


void dev::bacd::Dpos::verifyTransationVote(BlockHeader const& _bi)
{
    //处理投票交易确认
    m_dpos_cleint->dposVoteState()->currReset();
    size_t curr_num = _bi.number();
    LOG(m_logger) << EthYellow "************verifyTransationVote curr BlockNum:" << m_dpos_cleint->getCurrHeader().number() << "| create BlockNum:" << curr_num << EthYellow;
    if(curr_num > verifyVoteNum && m_dpos_cleint->dposVoteState()->isVerifyVoteTransation())
    {
        h256s _hash_t = m_dpos_cleint->getTransationsHashByBlockNum(size_t(curr_num - verifyVoteNum));
        if(_hash_t.empty())
            return;
        LOG(m_logger) << "************get transationHashs size:" << _hash_t.size();
        m_dpos_cleint->dposVoteState()->verifyVoteTransation(_bi, _hash_t);
        dealVoteDatas();
    }
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

void dev::bacd::Dpos::initDataByCurrBlock()
{
    BlockHeader const& _h = m_dpos_cleint->blockChain().info();
    m_dpos_context.clear();
    m_dpos_context = _h.dposContext();
}

bool dev::bacd::Dpos::isVarlitor(Address const & _addr)
{
    auto ret = std::find(m_dpos_context.curr_varlitor.begin(), m_dpos_context.curr_varlitor.end(), _addr);
    if(ret != m_dpos_context.curr_varlitor.end())
        return true;
    return false;
}

void dev::bacd::Dpos::printDposData(DposContext const& _d)
{
    LOG(m_logger) << EthYellow" DposData:" << EthYellow;
	_d.printData();
    
}