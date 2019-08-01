#include "Verify.h"
#include "Account.h"
#include <libbrccore/config.h>
#include <libdevcore/CommonIO.h>

using namespace dev;
using namespace dev::brc;

bool dev::brc::Verify::verify_standby(State const& state, int64_t block_time, const dev::Address &standby_addr, size_t varlitorInterval_time) const {

    std::map<Address, int64_t > records = state.block_record().m_last_time;
    std::vector<Address > minners;
    for(auto const& val: state.vote_data(SysVarlitorAddress)){
        minners.push_back(val.m_addr);
    }
    uint32_t  offset = (block_time / varlitorInterval_time) % minners.size();

    int beyond_num =0;      //out_of minner_rounds  if minner is offline , the beyond = time / (varlitorInterval_time * config::varlitorNum())
    auto ret_super = records.find(minners[offset]);
    if (ret_super != records.end()){
        beyond_num = (block_time - ret_super->second) / (varlitorInterval_time * config::varlitorNum());
        if (beyond_num <= config::minimum_cycle())
            return false;
    } else{
        cwarn << "not has minner:"<< minners[offset]<< " block records";
        return false;
    }

    // super offline
    // standby_addr has sorted
    std::vector<PollData> can_addr = state.vote_data(SysCanlitorAddress);

    auto ret_own = std::find(can_addr.begin(), can_addr.end(), standby_addr);
    if (ret_own == can_addr.end()) {
        testlog << "can't find standby:"<<standby_addr;
        return false;
    }

    //first find standby_own can to create
    // if not: return false  if can:will next verify
    auto ret_own_record = records.find(standby_addr);
    if (ret_own_record != records.end()){
        if((ret_own_record->second + config::varlitorNum() * varlitorInterval_time) > block_time )
            return false;
    }

    beyond_num = beyond_num - config::minimum_cycle();
    testlog << " beyond_num:" << beyond_num;

    // find the last_standby_addr for super_minner
    int64_t last_time =0;
    Address last_standby_addr = Address();
    int beyond_standby_num =0;               // out_of standby_rounds
    for(auto const& val: can_addr){
        if(records.count(val.m_addr)){
            uint32_t  index = (records[val.m_addr] / varlitorInterval_time) % minners.size();
            if (index == offset && last_time < records[val.m_addr]) {
                //find the last super's standby
                last_time = records[val.m_addr];
                last_standby_addr =val.m_addr;
                beyond_standby_num =  (block_time-records[val.m_addr])/ (varlitorInterval_time * config::varlitorNum());
            }
        }
    }
    testlog << "last_time:" << last_time << " last_standby_addr:"<<last_standby_addr << " beyond_standby_num:"<<beyond_standby_num;

    bool is_loop_last_standby = false;
    for(auto const& val: can_addr){
        if(!last_time) {
            if (beyond_num >=1 ) {
                if (val.m_addr == standby_addr) {
                    testlog << " not has old record ... will seal..." << standby_addr;
                    return true;
                }
                if (records.count(val.m_addr) && ((records[val.m_addr] + config::varlitorNum() * varlitorInterval_time) > block_time)){
                    --beyond_num ;
                }
            }
        } else{
            if (val.m_addr == standby_addr && beyond_standby_num >=1 ) {
                testlog<< " has old record ... begins standby will seal..."<< standby_addr;
                return true;
            }
            if(last_standby_addr == val.m_addr || is_loop_last_standby){
                --beyond_standby_num;
                is_loop_last_standby = true;
            }
        }
    }

    return false;
}