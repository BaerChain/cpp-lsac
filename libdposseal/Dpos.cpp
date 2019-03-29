#include "Dpos.h"
#include <libbrccore/TransactionBase.h>
#include <cstdlib>

dev::bacd::Dpos::Dpos()
{
    m_dpos_cleint =nullptr;
    m_last_create_block_time = 0;
}

dev::bacd::Dpos::~Dpos()
{
    m_dpos_cleint = nullptr;
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
}

void dev::bacd::Dpos::initConfigAndGenesis(ChainParams const & m_params)
{

    m_config.epochInterval = m_params.epochInterval;
    m_config.blockInterval = m_params.blockInterval;
    m_config.varlitorInterval = m_params.varlitorInterval;
    m_config.valitorNum = m_params.checkVarlitorNum;
    m_config.maxValitorNum = m_params.maxVarlitorNum;
    m_config.verifyVoteNum = m_params.verifyVoteNum;
    LOG(m_logger) << BrcYellow "dpos config:"<< m_config;
}

void dev::bacd::Dpos::init()
{
    BRC_REGISTER_SEAL_ENGINE(Dpos);
}

bool dev::bacd::Dpos::isBolckSeal(uint64_t _now)
{
    if(!CheckValidator(_now))
    {
        return false;
    } 
    return true;
}

bool dev::bacd::Dpos::checkDeadline(uint64_t _now)
{  
    //LOG(m_logger) <<"the curr and genesis hash:"<< m_dpos_cleint->getCurrBlockhash() << "||" << m_dpos_cleint->getGenesisHash();
    if(m_dpos_cleint->getCurrBlockhash() == m_dpos_cleint->getGenesisHash())
    {
        //parent 创世区块 不进行时间验证
        //验证人为指定验证人
        return true;
    }
    const BlockHeader _h = m_dpos_cleint->getCurrHeader();
    int64_t _last_time = _h.timestamp();
    if(_last_time <=0)
    {
        cwarn << " error! the block time is 0!";
        return false;
    }
    // 由于轮询时间过快，blockInterval 内多次进入
    int64_t comp_time = _now - m_config.blockInterval + 20;
    if(_last_time >= comp_time )
    {
        LOG(m_warnlog) << "the time is error ..";
        return false;
    }
    LOG(m_logger) << BrcYellow "begin to create new block! .." BrcReset <<"time:"<<_now;
    //得到每次出块的整数时间刻度，比较上次，现在和下次
    //系统时间算出的下一个出块时间点
    uint64_t next_slot = (_now + m_config.blockInterval - 1) / m_config.blockInterval * m_config.blockInterval;
    //当前块算出的上一个出块时间点
    uint64_t last_slot = (_last_time - 1) / m_config.blockInterval * m_config.blockInterval;
    //当前块算出即将出块时间点
    uint64_t curr_slot = last_slot + m_config.blockInterval;

    if(curr_slot <= _now || (next_slot - _now) <= 1)
    {
        return true;
    }
    LOG(m_logger) << BrcYellow"the slot time have some error! _now:"<< _now<< BrcReset;
    return false;
}

bool dev::bacd::Dpos::CheckValidator(uint64_t _now)
{
    cdebug << " into CheckValidator...";
    const BlockHeader _h = m_dpos_cleint->getCurrHeader();
	std::vector<Address> const& _currvar = _h.dposCurrVarlitors();  //_h.dposContext().curr_varlitor;
	std::vector<Address> const& _gennesis_var = m_dpos_cleint->getGenesisHeader().dposCurrVarlitors(); //dposContext().curr_varlitor;
    m_curr_varlitors.clear();
	m_curr_varlitors.assign(_currvar.begin(), _currvar.end());
    if(_currvar.empty())
    {
        LOG(m_warnlog) << "the m_dpos_context's curr_varlitor is empty and try to use genesis varlitor...";
        if(_gennesis_var.empty())
        {
            LOG(m_logger) << " the genesis varlitor is empty, can't to create block ...";
            return false;
        }
		m_curr_varlitors.clear();
		m_curr_varlitors.assign(_gennesis_var.begin(), _gennesis_var.end());
    }

    uint64_t offet = _now % m_config.epochInterval;       // 当前轮 进入了多时间
    LOG(m_logger) << "offet = _now % epochInterval:" << offet;
    offet /= m_config.varlitorInterval;
    LOG(m_logger) << "offet /= varlitorInterval:" << offet;
    offet %= m_curr_varlitors.size();
    LOG(m_logger) << "offet %= varlitors.size():" << offet;
    Address const& curr_valitor = m_curr_varlitors[offet]; //得到当验证人
    for (auto val : m_curr_varlitors)
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
    unsigned int prveslot = _last_time / m_config.epochInterval; //上一个块的周期
    unsigned int currslot = _now / m_config.epochInterval;   //当前即将出块的周期
    cdebug << BrcYellow"_last_time: " << _last_time << "|now:"<<_now;
    if(prveslot == currslot)
    {
        //处于相同周期 出块
        return ;
    }

    // 触发提出不合格验证人
    kickoutValidator();
    //统计投票
    countVotes();
    //打乱验证人顺序
    disorganizeVotes();
    
    LOG(m_logger) <<BrcYellow "******Come to new epoch, prevEpoch:"<< prveslot << "nextEpoch:" << currslot<< BrcYellow;
}

void dev::bacd::Dpos::kickoutValidator()
{
    //踢出不合格的候选人 不能成为新一轮的出块者
    kickoutcanlidates();
    //剔除不合格 当下一轮验证人+ 创世配置验证人 > maxValitorNum 触发
    
}

void dev::bacd::Dpos::kickoutcanlidates()
{
    
}

void dev::bacd::Dpos::countVotes()
{
    //统计投票
	std::vector<Address> _v;
	m_dpos_cleint->getEletorsByNum(_v, m_config.maxValitorNum);
	if(_v.empty())
		return;
	m_curr_varlitors.clear();
	m_curr_varlitors.assign(_v.begin(), _v.end());
}

void dev::bacd::Dpos::disorganizeVotes()
{
    //随机打乱验证人
    int range = m_curr_varlitors.size();
    // rand()%(Y-X+1)+X [X,Y]
    //rand()%range
    //使用 parent hash 种子
    //size_t parent_hash =(size_t)m_dpos_cleint->getCurrBlockhash();
    srand(utcTimeMilliSec());
    for(int i=0; i < range; i++)
    {
        int j = rand() % range;
        Address temp_addr = m_curr_varlitors[i];
		m_curr_varlitors[i] = m_curr_varlitors[j];
		m_curr_varlitors[j] = temp_addr;
    }
}
