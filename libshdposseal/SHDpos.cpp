#include "SHDpos.h"
#include <libbrccore/TransactionBase.h>
#include <libdevcore/Address.h>
#include <libdevcore/Exceptions.h>
#include <libbrccore/config.h>
#include <cstdlib>

dev::bacd::SHDpos::SHDpos()
{
    m_dpos_cleint = nullptr;
    m_next_block_time = 0;
    m_last_block_time = 0;
}

dev::bacd::SHDpos::~SHDpos()
{
    //m_dpos_cleint = nullptr;
	m_badblock_db.release();
}

void dev::bacd::SHDpos::generateSeal(BlockHeader const& _bi)
{
    BlockHeader header(_bi);
    header.setSeal(NonceField, h64{0});
    header.setSeal(MixHashField, h256{0});
    RLPStream ret;
    header.streamRLP(ret);
    if (m_onSealGenerated)
    {
        m_onSealGenerated(ret.out());
    }

	updateBadBlockData();
}


void dev::bacd::SHDpos::populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const
{
	SealEngineBase::populateFromParent(_bi, _parent);
    //TODO
    //get create block time by time
	int64_t _time1 = utcTimeMilliSec() / m_config.blockInterval * m_config.blockInterval;
	int64_t _time2 = _parent.timestamp() / m_config.blockInterval * m_config.blockInterval + m_config.blockInterval;
	_bi.setTimestamp(_time1 > _time2 ? _time1 : _time2);

}


void dev::bacd::SHDpos::verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent /*= BlockHeader()*/, bytesConstRef _block /*= bytesConstRef()*/) const {
    // will verify sign and creater

    auto start = utcTimeMilliSec();
    CLATE_LOG << "SHDpos time begin " << start;
	SealEngineBase::verify(_s, _bi, _parent, _block);
    std::vector<Address> var_v;

    if(m_dpos_cleint){
        m_dpos_cleint->getCurrCreater(CreaterType::Varlitor, var_v);
        auto ret = find(var_v.begin(), var_v.end(), _bi.author());
        if(ret == var_v.end()) {
            uint64_t  offet = (_bi.timestamp() + 5) / m_config.varlitorInterval;
            offet %= var_v.size();
//            if(!verify_standby(var_v[offet], _bi.author())){
//                BOOST_THROW_EXCEPTION(InvalidAutor() << errinfo_wrongAddress(toString(_bi.author()) + " Invalid to seal block"));
//            }
        }
    }
    CLATE_LOG << "SHDpos time end " << utcTimeMilliSec() - start << " ms";
}

void dev::bacd::SHDpos::initConfigAndGenesis(ChainParams const & m_params)
{
    m_config.epochInterval = m_params.epochInterval;
    m_config.blockInterval = m_params.blockInterval;
    m_config.varlitorInterval = m_params.varlitorInterval;
    LOG(m_logger) << BrcYellow "dpos config:" << m_config;
}

void dev::bacd::SHDpos::init()
{
    BRC_REGISTER_SEAL_ENGINE(SHDpos);
}

bool dev::bacd::SHDpos::isBolckSeal(uint64_t _now)
{
    if (!CheckValidator(_now))
    {
        return false;
    }
    return true;
}


bool dev::bacd::SHDpos::checkDeadline(uint64_t _now)
{  
    //LOG(m_logger) <<"the curr and genesis hash:"<< m_dpos_cleint->getCurrBlockhash() << "||" << m_dpos_cleint->getGenesisHash();
    if(m_dpos_cleint->getCurrBlockhash() == m_dpos_cleint->getGenesisHash())
    {
        // parent Genesis
        if (!m_next_block_time && !m_last_block_time)
        {
            m_last_block_time = (_now / m_config.blockInterval) * m_config.blockInterval + 1;
            m_next_block_time = m_last_block_time + m_config.blockInterval;
        }
    }
    else
    {
        const BlockHeader _h = m_dpos_cleint->getCurrHeader();
        m_last_block_time = _h.timestamp();
    }

    if (m_last_block_time <= 0)
    {
        cwarn << " error! the block time is 0!";
        return false;
    }

    if (_now < uint64_t(m_next_block_time))
        return false;

    //得到每次出块的整数时间刻度，比较上次，现在和下次
    //get next Block time from systime
    uint64_t next_slot =
        (_now / m_config.blockInterval) * m_config.blockInterval + m_config.blockInterval;
    //当前块算出的上一个出块时间点
    uint64_t last_slot = (m_last_block_time) / m_config.blockInterval * m_config.blockInterval;
    //当前块算出即将出块时间点
    uint64_t curr_slot = last_slot + m_config.blockInterval;

    if (curr_slot <= _now || (next_slot - _now) <= 1)
    {
        m_next_block_time = next_slot;
        return true;
    }
    return false;
}
void dev::bacd::SHDpos::workLoop()
{
	while(isWorking())
	{
		//TODO dell net message
		std::pair<bool, SHDposMsgPacket> ret = m_msg_queue.tryPop(5);
		if(!ret.first)
		{
			continue;
		}
		LOG(m_logger) << " get mesg: ||" << ret.second.packet_id << ret.second.node_id ;
		switch((SHDposPacketType)ret.second.packet_id)
		{
		case SHDposPacketType::SHDposDataPacket:
		{
		    // init the new SHDpos data from other peer
            // TODO
        }
        break;
        case SHDposPacketType::SHDposBadBlockPacket:
        {
            // int this get a new badBlock
            // will to verfy the badBlock if verfy feild will to record the bad log into dataBase
            // TODO
			SH_DposBadBlock  _data;
			_data.populate(RLP(ret.second.data));
			LOG(m_logger) << "get BadBlockData msg :" << _data;
            // check sgin
			if(!verifySignBadBlock(_data.m_badBlock, _data.m_signData))
				return;
			BlockHeader _h = BlockHeader(_data.m_badBlock);
			auto ret = m_badVarlitors.find(_h.author());
            if(ret == m_badVarlitors.end())
				m_badVarlitors[_h.author()] = { _h.hash(), _data.m_badBlock, _data.m_addr };
			else
			{
				if(ret->second.findVerifyVarlitor(_h.hash(), _data.m_addr))
					return;
				ret->second.insert(_h.hash(), _data.m_badBlock, _data.m_addr);
			}	
			RLPStream _s;
			_data.streamRLPFields(_s);
			brocastMsg(SHDposBadBlockPacket, _s);

			insertUpdataSet(_h.author(), BadBlockDatas);
			updateBadBlockData();
		}
        break;
        default:
            break;
        }
    }
}


void dev::bacd::SHDpos::sealAndSend(NodeID const& _nodeid, SHDposPacketType _type, RLPStream const& _msg_s)
{
    // seal and send
    RLPStream msg_ts;
    auto h = m_host.lock();  // weak_ptr need to  unlock to share_ptr
    h->hostFace()->prep(_nodeid, name(), msg_ts, _type, 1).append(_msg_s.out());  // seal ID....
    h->hostFace()->sealAndSend(_nodeid, msg_ts);
    cdebug << "SH-Dpos Send[" << _type << "] to " << _nodeid;
}

void dev::bacd::SHDpos::brocastMsg(SHDposPacketType _type, RLPStream& _msg_s)
{
    // brocast msg  the sealAndSend() called back
    auto h = m_host.lock();
    h->hostFace()->foreachPeer(h->name(), [&](NodeID const& _nodeId) {
        sealAndSend(_nodeId, _type, _msg_s);
        cdebug << "brocastMsg ... NodeId:" << _nodeId << "type" << _type;
        return true;
    });
}

bool dev::bacd::SHDpos::CheckValidator(uint64_t _now)
{
	Timer _timer;
	m_curr_varlitors.clear();
	if(!m_dpos_cleint)
        return false;
	m_dpos_cleint->getCurrCreater(CreaterType::Varlitor, m_curr_varlitors);
	if(m_curr_varlitors.empty())
	{
		cerror << " not have Varlitors to create block!";
		return false;
	}

    uint64_t  offet = _now / m_config.varlitorInterval;
    offet %= m_curr_varlitors.size();
    if(m_curr_varlitors[offet] == m_dpos_cleint->author())
        return true;

    if(std::find(m_curr_varlitors.begin(), m_curr_varlitors.end(), m_dpos_cleint->author()) != m_curr_varlitors.end())
        return false;

   BlockHeader h = m_dpos_cleint->getCurrHeader();
   //testlog << h.number() << " "<< m_dpos_cleint->author();
   if (h.number() <= dev::brc::config::varlitorNum() * dev::brc::config::minimum_cycle()){
       return false;
   }
//   return verify_standby(m_curr_varlitors[offet], m_dpos_cleint->author());
    return true;
}

bool dev::bacd::SHDpos::verify_standby(const dev::Address &super_addr, const dev::Address &own_addr) const{

    if (!m_dpos_cleint)
        return false;
    return m_dpos_cleint->verify_standby(super_addr, own_addr);
}

bool dev::bacd::SHDpos::chooseBlockAddr(Address const& _addr, bool _isok)
{
	//节点地址与出块地址相同，且该地址不被惩罚，返回true出块
	if (m_dpos_cleint->author() == _addr && _isok == false)
    {
        return true;
    }
	//节点地址与出块地址相同，且该地址要被惩罚
    if (m_dpos_cleint->author() == _addr && _isok == true)
    {
        // 当前惩罚的块数+1
        punishBlockVarlitors(_addr);
        return false;
    }
    if (m_dpos_cleint->author() != _addr && _isok == true)
	{
		// 拿到不被惩罚的候选人地址，并存储需要惩罚的候选人地址
		Address _candidate;
		size_t i = 0;
		auto it = m_curr_candidate.begin();
		while (it != m_curr_candidate.end() && i != 1)
		{
			if (isCurrBlock(*it))
			{
				m_curr_punishVandidate.insert(*it);
				it++;
			}
			else
			{
				_candidate = *it;
				i += 1;
			}
		}

		// 如果需要惩罚的候选人地址和候选人地址数量相同，返回false
		if (m_curr_punishVandidate.size() == m_curr_candidate.size())
		{
			return false;
		}

		// 候选人地址替换出块，同时惩罚需要惩罚的候选人地址；
		if (m_dpos_cleint->author() == _candidate)
		{
			// 被惩罚的候选人，当前惩罚的出块数+1
			addCandidatePunishBlock();
			return true;
		}
	}
    return false;
}

void dev::bacd::SHDpos::addCandidatePunishBlock()
{
    for (auto val : m_curr_punishVandidate)
    {
        punishBlockVarlitors(val);
    }
}

// 判断地址_curr是否需要惩罚 true表示需要惩罚
bool dev::bacd::SHDpos::isCurrBlock(Address const& _curr)
{
    auto iter = m_punishVarlitor.find(_curr);
    if (iter != m_punishVarlitor.end())
    {
        if (iter->second.m_isPunish && (iter->second.m_badBlockNum >= BadNum_Two && iter->second.m_badBlockNum < BadNum_Three))
        {
            return true;
        }
    }
    return false;
}

// 当前被惩罚出块数+1
void dev::bacd::SHDpos::punishBlockVarlitors(Address const& _addr)
{
    auto it = m_punishVarlitor.find(_addr);
    it->second.m_currentEcoph += 1;
    if (it->second.m_punishEcoph == it->second.m_currentEcoph)
    {
        it->second.m_isPunish = false;
        it->second.m_punishEcoph = 0;
        it->second.m_currentEcoph = 0;
    }
	insertUpdataSet(_addr, BadBlockPushs);
}

// 惩罚出块人不能出块
bool dev::bacd::SHDpos::punishBlockCandidate(Address& _addr)
{
    auto it = m_punishVarlitor.find(_addr);
    if (it == m_punishVarlitor.end())
    {
        return true;
    }
    else
    {
        if (!it->second.m_isPunish)
        {
            return true;
        }
        if (it->second.m_isPunish &&
            (it->second.m_badBlockNum >= BadNum_Two && it->second.m_badBlockNum < BadNum_Three))
        {
            it->second.m_currentEcoph += 1;
            if (it->second.m_punishEcoph == it->second.m_currentEcoph)
            {
                it->second.m_isPunish = false;
                it->second.m_punishEcoph = 0;
                it->second.m_currentEcoph = 0;
            }
        }
        return false;
    }
}

void dev::bacd::SHDpos::tryElect(uint64_t _now)
{
    //这里 验证人已经通过 尝试统计投票
    //尝试后继续出块 则继续出块
	if(!m_config.epochInterval)
		return;
    const BlockHeader _h = m_dpos_cleint->getCurrHeader();
    uint64_t _last_time = _h.timestamp();
    unsigned int prveslot = _last_time / m_config.epochInterval;  //上一个块的周期
    unsigned int currslot = _now / m_config.epochInterval;        //当前即将出块的周期
    //cdebug << BrcYellow "_last_time: " << _last_time << "|now:" << _now;
    if (prveslot == currslot)
    {
        //处于相同周期 出块
        return;
    }
	cdebug << BrcYellow "init0 new epoch... _last_time: " << _last_time << "|now:" << _now;
    //统计投票
    countVotes();

}


//候选人替换惩罚的验证人出块 _replaceNum有几人在本轮不能出块
void dev::bacd::SHDpos::candidateReplaceVarlitors(size_t& _replaceNum)
{
    size_t i = 0;
    auto val = m_curr_candidate.begin();
    while (val != m_curr_candidate.end() && i < _replaceNum)
    {
        if (isCandidateBlock(*val))
        {
            m_curr_varlitors.push_back(*val);
            m_curr_candidate.erase(val++);
            i += 1;
        }
        else
        {
            ++val;
        }
    }
}

//判断候选人本轮是否也是在惩罚,true表示能出块，false表示不能出块
bool dev::bacd::SHDpos::isCandidateBlock(Address _addr)
{
    auto ret = m_punishVarlitor.find(_addr);
    if (ret == m_punishVarlitor.end())
    {
        return true;
    }
    else
    {
        // 如果候选人也处于惩罚状态，当前被惩罚轮数+1
        if (ret->second.m_isPunish &&
            (ret->second.m_badBlockNum >= BadNum_One && ret->second.m_badBlockNum < BadNum_Two))
        {
            ret->second.m_currentEcoph += 1;
            if (ret->second.m_punishEcoph == ret->second.m_currentEcoph)
            {
                ret->second.m_isPunish = false;
                ret->second.m_punishEcoph = 0;
                ret->second.m_currentEcoph = 0;
            }
            return false;
        }
    }
    return true;
}

// 踢出出块人使其一轮不能出块
size_t dev::bacd::SHDpos::kickoutVarlitors()
{
    size_t eraseNum;
    for (auto iter = m_punishVarlitor.begin(); iter != m_punishVarlitor.end(); iter++)
    {
        if (iter->second.m_isPunish && (iter->second.m_badBlockNum >= BadNum_One && iter->second.m_badBlockNum < BadNum_Two))
        {
            auto it = m_curr_varlitors.begin();
            while (it != m_curr_varlitors.end())
            {
                if (*it == iter->first)
                {
                    eraseNum += 1;
                    iter->second.m_currentEcoph += 1;
                    if (iter->second.m_punishEcoph == iter->second.m_currentEcoph)
                    {
                        iter->second.m_isPunish = false;
                        iter->second.m_punishEcoph = 0;
                        iter->second.m_currentEcoph = 0;
                    }
                    m_curr_varlitors.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
    }
    m_notBlockNum = eraseNum;
    return m_notBlockNum;
}


void dev::bacd::SHDpos::openBadBlockDB(boost::filesystem::path const& _dbPath)
{
	m_badblock_db = db::DBFactory::create(db::DatabaseKind::LevelDB, _dbPath / boost::filesystem::path("bad-block"));
}


void dev::bacd::SHDpos::initBadBlockByDB()
{
	//std::function<bool(db::Slice _key, db::Slice _v)> _f = load_key_value;
	m_badblock_db->forEach([&](db::Slice _key, db::Slice _v){ return load_key_value(_key, _v); });
}

void dev::bacd::SHDpos::onDposMsg(NodeID _nodeid, unsigned _id, RLP const & _r)
{
	if(_id < SHDposPacketCount && _id >= SHDposStatuspacket)
	{
		cdebug << "onRaftMsg: id=" << _id << ",from=" << _nodeid;
		m_msg_queue.push(SHDposMsgPacket(_nodeid, _id, _r[0].data()));
	}
	else
	{
		cwarn << "Recv an illegal msg, id=" << _id << "  and the max_id:" << SHDposPacketCount - 1;
	}
}

void dev::bacd::SHDpos::requestStatus(NodeID const & _nodeID, u256 const & _peerCapabilityVersion)
{
    // connet net
    // TODO
    LOG(m_logger) << " connnt SH-Dpos net...";
}


void dev::bacd::SHDpos::countVotes()
{
    //统计投票
    m_curr_varlitors.clear();

    std::vector<Address> _blackList;
    addBlackList(_blackList);
    m_dpos_cleint->getEletorsByNum(m_curr_varlitors, m_config.totalElectorNum, _blackList);

    // Identify the verlitors and candidate
    getVarlitorsAndCandidate(m_curr_varlitors);
}

void dev::bacd::SHDpos::getVarlitorsAndCandidate(std::vector<Address>& _curr_varlitors)
{
    std::vector<Address> _v = _curr_varlitors;
    _curr_varlitors.clear();
    m_curr_candidate.clear();

	auto index =dev::brc::config::varlitorNum();
	for(auto v : _v)
	{
		if(index > 0)
		{
			index--;
			_curr_varlitors.push_back(v);
		}
		else
			m_curr_candidate.push_back(v);
	}
}

// 超过12个坏块，加入黑名单
void dev::bacd::SHDpos::addBlackList(std::vector<Address>& _v)
{
    if (!m_punishVarlitor.empty())
    {
        auto iter = m_punishVarlitor.begin();
        while (iter != m_punishVarlitor.end())
        {
            if (iter->second.m_badBlockNum >= BadNum_Three)
            {
                // m_blackList.insert(std::make_pair<Address, PunishBadBlock>(iter->first,
                // iter->second));
                m_blackList[iter->first] = iter->second;
                _v.push_back(iter->first);
                m_punishVarlitor.erase(iter++);
				insertUpdataSet(iter->first, BadBlockPushs);
            }
            else
            {
                ++iter;
            }
        }
    }
}

void dev::bacd::SHDpos::disorganizeVotes()
{
    //随机打乱验证人
    int range = m_curr_varlitors.size();
    // rand()%(Y-X+1)+X [X,Y]
    // rand()%range
    // srand(utcTimeMilliSec());
	int64_t _time = m_dpos_cleint->getCurrHeader().timestamp();
	if(!_time)
		_time = 1546272000000;
    srand(m_dpos_cleint->getCurrHeader().timestamp());
    for (int i = 0; i < range; i++)
    {
        int j = rand() % range;
        Address temp_addr = m_curr_varlitors[i];
        m_curr_varlitors[i] = m_curr_varlitors[j];
        m_curr_varlitors[j] = temp_addr;
    }
}

void dev::bacd::SHDpos::dellImportBadBlock(bytes const& _block)
{
    // TODO
    // 1. verfy the sender sgin
    // 2. verfy the bad Block if the Block is bad will to add record

    // sign badBlock
    // Secret _key = m_dpos_cleint->getVarlitorSecret(m_dpos_cleint->author());
    Secret _key = m_dpos_cleint->getVarlitorSecret(m_dpos_cleint->author());
    if (_key)
    {
        // add local
        addBadBlockLocal(_block, m_dpos_cleint->author());
        // send to net by  brocast
        sendBadBlockToNet(_block);
        // add bad block num
        addBadBlockInfo(BlockHeader(_block, HeaderData).author(), 1);
    }
    cwarn << "not find the key, can not block!";

}

// 添加出块人出的坏块信息
void dev::bacd::SHDpos::addBadBlockInfo(Address _addr, size_t _badBlockNums)
{
    BadBlockPunish data;
    auto iter = m_punishVarlitor.find(_addr);
    if (iter != m_punishVarlitor.end())
    {
        iter->second.m_badBlockNum += _badBlockNums;
        size_t _num = iter->second.m_badBlockNum;

        auto it = m_badPunish.m_badBlockNumPunish.find(_num);
        if (it != m_badPunish.m_badBlockNumPunish.end())
        {
            data.m_badBlockNum = _num;
            data.m_punishEcoph = it->second;
            data.m_isPunish = true;
            m_punishVarlitor[_addr] = data;
            cwarn << "[*3*]: peoduce bad block addr: " << iter->first
                  << "m_badBlockNum: " << iter->second.m_badBlockNum
                  << "m_currentEcoph:" << iter->second.m_currentEcoph
                  << "m_punishEcoph: " << iter->second.m_punishEcoph << "m_isPunish"
                  << iter->second.m_isPunish;
        }
        else
        {
            cwarn << "[*2*]: peoduce bad block addr: " << iter->first
                  << "m_badBlockNum: " << iter->second.m_badBlockNum
                  << "m_currentEcoph:" << iter->second.m_currentEcoph
                  << "m_punishEcoph: " << iter->second.m_punishEcoph << "m_isPunish"
                  << iter->second.m_isPunish;
        }
    }
    else
    {
        data.m_badBlockNum = _badBlockNums;
        m_punishVarlitor[_addr] = data;
        cwarn << "[*1*]: peoduce bad block addr: " << iter->first
              << "m_badBlockNum: " << iter->second.m_badBlockNum
              << "m_currentEcoph:" << iter->second.m_currentEcoph
              << "m_punishEcoph: " << iter->second.m_punishEcoph << "m_isPunish"
              << iter->second.m_isPunish;
    }
}

dev::SignatureStruct dev::bacd::SHDpos::signBadBlock(const Secret &sec, bytes const& _badBlock)
{
	RLPStream stream;
	stream << _badBlock;                         
	auto hash = sha3(stream.out());
	return SignatureStruct(sign(sec, hash));
}

bool dev::bacd::SHDpos::verifySignBadBlock( bytes const& _badBlock, SignatureStruct const& _signData)
{
    // TODO
    // SHDPOS will to verfiy _badBlock sgin
	RLPStream stream;
	stream << _badBlock;
	auto hash = sha3(stream.out());
	Address _createrAddr = BlockHeader(_badBlock).author();
	auto p = recover(_signData, hash);
	if(!p)
		return false;
	return  _createrAddr == right160(dev::sha3(bytesConstRef(p.data(), sizeof(p))));
}

void dev::bacd::SHDpos::addBadBlockLocal(bytes const& _b, Address const& _verifyAddr)
{
    BlockHeader _h = BlockHeader(_b);
    Address _createrAddr = _h.author();
    auto ret = m_badVarlitors.find(_createrAddr);
    if (ret != m_badVarlitors.end())
        ret->second.insert(_h.hash(), _b, _verifyAddr);
	else
	{
		BadBlocksData data;
		data.insert(_h.hash(), _b, _verifyAddr);
		m_badVarlitors[_createrAddr] = data;
	}

	insertUpdataSet(_createrAddr, badBlockAll);
	updateBadBlockData();
}

void dev::bacd::SHDpos::sendBadBlockToNet(bytes const& _block)
{
	Secret _key = m_dpos_cleint->getVarlitorSecret(m_dpos_cleint->author());
	SH_DposBadBlock _badBlock;
	_badBlock.m_signData = signBadBlock(_key, _block);
	_badBlock.m_addr = m_dpos_cleint->author();
	_badBlock.m_badBlock = _block;

	RLPStream _s;
	_badBlock.streamRLPFields(_s);
	brocastMsg(SHDposBadBlockPacket, _s);
}

dev::db::Slice dev::bacd::SHDpos::toSlice(Address const& _h, unsigned _sub)
{
#if ALL_COMPILERS_ARE_CPP11_COMPLIANT
	static thread_local FixedHash<21> h = _h;
	h[20] = (uint8_t)_sub;
	return (db::Slice)h.ref();
#else
	static boost::thread_specific_ptr<FixedHash<21>> t_h;
	if(!t_h.get())
		t_h.reset(new FixedHash<21>);
	*t_h = FixedHash<21>(_h);
	(*t_h)[20] = (uint8_t)_sub;
	return (db::Slice) t_h->ref();
#endif //ALL_COMPILERS_ARE_CPP11_COMPLIANT
}

bool dev::bacd::SHDpos::load_key_value(db::Slice _key, db::Slice _val)
{
	if(_key.count() <= 20)
		return true;
	try
	{
		BadBlockType _type = (BadBlockType)_key[20];
		bytes _a = _key.cropped(0, Address::size).toBytes();
		Address _addr = Address(_a);
		cwarn << _addr;
		populateBadBlock( RLP(_val.toString()), _addr, _type);

	}
	catch(Exception const& ex)
	{
		cerror << ex.what();
		return false;
	}
	return true;
}


void dev::bacd::SHDpos::streamBadBlockRLP(RLPStream& _s, Address _addr, BadBlockType _type)
{
	if(_type == BadBlockPushs)
	{
		// m_blackList
		RLPStream _ls;
		auto ret = m_blackList.find(_addr);
		if(ret != m_blackList.end())
			ret->second.streamRLP(_ls);
		_s << _ls.out();
		// m_punishVarlitor
		RLPStream _vs;
		auto ret1 = m_punishVarlitor.find(_addr);
		if(ret1 != m_punishVarlitor.end())
			ret->second.streamRLP(_vs);
		_s << _vs.out();
	}
    if(_type == BadBlockDatas )
	{
        //m_badVarlitors
		auto ret = m_badVarlitors.find(_addr);
	    if(ret != m_badVarlitors.end())
		{
			ret->second.streamRLP(_s);
		}
	}
}


void dev::bacd::SHDpos::populateBadBlock(RLP const& _r, Address _addr, BadBlockType _type)
{
	try
	{
		if(_type == BadBlockPushs)
		{
			if(_r.itemCount() != 2)
				return;
			BadBlockPunish _lp;
			_lp.populate(_r[0]);
			m_blackList[_addr] = _lp;

			BadBlockPunish _vp;
			_vp.populate(_r[1]);
			m_punishVarlitor[_addr] = _vp;
		}
        if(_type == BadBlockDatas)
		{
			BadBlocksData _b;
			_b.populate(_r[0]);
			m_badVarlitors[_addr] = _b;
		}
	}
	catch(Exception&)
	{
		throw;
	}
    
}

void dev::bacd::SHDpos::updateBadBlockData()
{
    for (auto val : m_up_set)
    {
        if(val.second != badBlockAll)
		{
			RLPStream _s;
			streamBadBlockRLP(_s, val.first, val.second);
			insertBadBlock(toSlice(val.first, val.second), toString(_s.out()));
		}
		else
		{
			RLPStream _s;
			streamBadBlockRLP(_s, val.first, BadBlockPushs);
			insertBadBlock(toSlice(val.first, BadBlockPushs), toString(_s.out()));

			RLPStream _ss;
			streamBadBlockRLP(_ss, val.first, BadBlockDatas);
			insertBadBlock(toSlice(val.first, BadBlockDatas), toString(_ss.out()));
		}
		
    }
	m_up_set.clear();
}

void dev::bacd::SHDpos::insertUpdataSet(Address const& _addr, BadBlockType _type)
{
	auto ret = m_up_set.find(_addr);
    if(ret == m_up_set.end())
	{
		m_up_set[_addr] = _type;
		return;
	}
	if(_type != ret->second)
		ret->second = badBlockAll;
}
