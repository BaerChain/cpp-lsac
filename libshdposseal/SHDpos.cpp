#include "SHDpos.h"
#include <libbrccore/TransactionBase.h>
#include <libdevcore/Address.h>
#include <libdevcore/Exceptions.h>
#include <libbrccore/config.h>
#include <cstdlib>

dev::bacd::SHDpos::SHDpos() {
    m_dpos_cleint = nullptr;
    m_next_block_time = 0;
    m_last_block_time = 0;
}

dev::bacd::SHDpos::~SHDpos() {
    //m_dpos_cleint = nullptr;
}

void dev::bacd::SHDpos::generateSeal(BlockHeader const &_bi) {
    BlockHeader header(_bi);
    header.setSeal(NonceField, h64{0});
    header.setSeal(MixHashField, h256{0});
    RLPStream ret;
    header.streamRLP(ret);
    if (m_onSealGenerated) {
        m_onSealGenerated(ret.out());
    }
}


void dev::bacd::SHDpos::populateFromParent(BlockHeader &_bi, BlockHeader const &_parent) const {
    SealEngineBase::populateFromParent(_bi, _parent);
    //TODO
    //get create block time by time
    int64_t _time1 = utcTimeMilliSec() / m_config.blockInterval * m_config.blockInterval;
    int64_t _time2 = _parent.timestamp() / m_config.blockInterval * m_config.blockInterval + m_config.blockInterval;
    _bi.setTimestamp(_time1 > _time2 ? _time1 : _time2);

}


void dev::bacd::SHDpos::verify(Strictness _s, BlockHeader const &_bi, BlockHeader const &_parent /*= BlockHeader()*/,
                               bytesConstRef _block /*= bytesConstRef()*/) const {
    // will verify sign and creater
    SealEngineBase::verify(_s, _bi, _parent, _block);
}

void dev::bacd::SHDpos::initConfigAndGenesis(ChainParams const &m_params) {
    m_config.epochInterval = m_params.epochInterval;
    m_config.blockInterval = m_params.blockInterval;
    m_config.varlitorInterval = m_params.varlitorInterval;
    LOG(m_logger) << BrcYellow "dpos config:" << m_config;
}

void dev::bacd::SHDpos::init() {
    BRC_REGISTER_SEAL_ENGINE(SHDpos);
}

bool dev::bacd::SHDpos::isBolckSeal(uint64_t _now) {
    if (!CheckValidator(_now)) {
        return false;
    }
    return true;
}


bool dev::bacd::SHDpos::checkDeadline(uint64_t _now) {
    //LOG(m_logger) <<"the curr and genesis hash:"<< m_dpos_cleint->getCurrBlockhash() << "||" << m_dpos_cleint->getGenesisHash();
    if (m_dpos_cleint->getCurrBlockhash() == m_dpos_cleint->getGenesisHash()) {
        // parent Genesis
        if (!m_next_block_time && !m_last_block_time) {
            m_last_block_time = (_now / m_config.blockInterval) * m_config.blockInterval + 1;
            m_next_block_time = m_last_block_time + m_config.blockInterval;
        }
    } else {
        const BlockHeader _h = m_dpos_cleint->getCurrHeader();
        m_last_block_time = _h.timestamp();
    }

    if (m_last_block_time <= 0) {
        cwarn << " error! the block time is 0!";
        return false;
    }

    if (_now < uint64_t(m_next_block_time + 50))
        return false;

    //得到每次出块的整数时间刻度，比较上次，现在和下次
    //get next Block time from systime
    uint64_t next_slot =
            (_now / m_config.blockInterval) * m_config.blockInterval + m_config.blockInterval;
    //当前块算出的上一个出块时间点
    uint64_t last_slot = (m_last_block_time) / m_config.blockInterval * m_config.blockInterval;
    //当前块算出即将出块时间点
    uint64_t curr_slot = last_slot + m_config.blockInterval;

    if (curr_slot <= _now || (next_slot - _now) <= 1) {
        m_next_block_time = next_slot;
        return true;
    }
    return false;
}

void dev::bacd::SHDpos::workLoop() {
    // while (isWorking()) {
    //     //TODO dell net message
     
    // }
}

bool dev::bacd::SHDpos::CheckValidator(uint64_t _now) {
    if (!m_dpos_cleint)
        return false;
    m_dpos_cleint->getCurrCreater(CreaterType::Varlitor, m_curr_varlitors);
    if (m_curr_varlitors.empty()) {
        cerror << " not have Varlitors to create block!";
        return false;
    }
    //testlog << m_curr_varlitors;
    uint64_t offet = _now / m_config.varlitorInterval;
    offet %= m_curr_varlitors.size();
    if (m_curr_varlitors[offet] == m_dpos_cleint->author())
        return true;

    if (std::find(m_curr_varlitors.begin(), m_curr_varlitors.end(), m_dpos_cleint->author()) != m_curr_varlitors.end())
        return false;

    BlockHeader h = m_dpos_cleint->getCurrHeader();
    if (h.number() <= dev::brc::config::varlitorNum() * dev::brc::config::minimum_cycle()) {
        return false;
    }
    /// fork about replace
    if(h.number() >= config::replaceMinerHeight()) {
        return verify_standby(_now - (_now % m_config.varlitorInterval), m_dpos_cleint->author());
    } else{
        return false;
    }
}

bool dev::bacd::SHDpos::verify_standby(int64_t block_time, Address const &own_addr) const {

    if (!m_dpos_cleint)
        return false;
    return m_dpos_cleint->verify_standby(block_time, own_addr, m_config.varlitorInterval);
}


