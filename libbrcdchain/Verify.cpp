#include "Verify.h"
#include "Account.h"
#include <libbrccore/config.h>
#include <libdevcore/CommonIO.h>

using namespace dev;
using namespace dev::brc;

bool dev::brc::Verify::verify_standby(State const& state, int64_t block_time, const dev::Address &_standby_addr,
                            size_t varlitorInterval_time, bool fork_change_miner /*= false*/) const {
    //return false;
    //cnote << " into verify standby ...";
    std::map<Address, int64_t > records = state.block_record().m_last_time;
    std::vector<Address > minners;
    for(auto const& val: state.vote_data(SysVarlitorAddress)){
        minners.push_back(val.m_addr);
    }
    if(minners.empty() || records.empty()){
        cwarn << " not has super miner or seal block records";
        return false;
    }
    uint32_t  offset = (block_time / varlitorInterval_time) % minners.size();

    // changeMiner fork
    if(fork_change_miner){
        //change miner data  A  B  C  D
        /**  changeMapping
         * 1. A: <0,0>                                         2. A -> B :A:<a, b>  B: <a,b>
           3. B -> C :A:<a, c>  B: <0,0>  C:<a,c>              4. C -> D :A:<a, d>  C: <0,0>  D:<a,d>
           5. D -> A :A:<0, 0>  D: <0,0>
         **/
        auto mapping_miner = state.replaceMiner(minners[offset]);
        if(mapping_miner.second != Address() && records.find(minners[offset]) == records.end()){
            ctrace << "miner changed  replace minners ...";
            std::replace(minners.begin(), minners.end(), minners[offset], mapping_miner.first);
        }
    }

    int beyond_num =0;      //out_of minner_rounds  if minner is offline , the beyond = time / (varlitorInterval_time * config::varlitorNum())
    auto ret_super = records.find(minners[offset]);
    if (ret_super != records.end()) {
        beyond_num = (block_time - ret_super->second) / (varlitorInterval_time * config::varlitorNum());
        // if the last offlion time smaller config::minimum_cycle() * one_round_time
        if (beyond_num <= config::minimum_cycle()) {
            ctrace << "the super miner offline time is not out offline_cycle the standby not to create";
            return false;
        }
    } else{
        cwarn <<" can not find super :"<< minners[offset] << " seal records this time point can not to seal";
        return false;
    }

    // super offline   standby_addr has sorted
    std::vector<PollData> standby_addrs = state.vote_data(SysCanlitorAddress);

    auto ret_own = std::find(standby_addrs.begin(), standby_addrs.end(), _standby_addr);
    if (ret_own == standby_addrs.end()) {
        cwarn << "can't find standby:"<<_standby_addr;
        return false;
    }

    ///standby changeMiner
    Address standby_addr = _standby_addr;
    if(fork_change_miner){
        auto mapping_miner = state.replaceMiner(standby_addr);
        if(mapping_miner.second != Address() && records.find(standby_addr) == records.end()){
            ctrace << "standby_miner changed  replace minners ...";
            std::replace(standby_addrs.begin(), standby_addrs.end(), standby_addr, mapping_miner.first);
            standby_addr = mapping_miner.first;
        }
    }

    //first find standby_own can to create, the last create_time must bigger one round_time
    // if not: return false  if can:will next verify
    auto ret_own_record = records.find(standby_addr);
    if (ret_own_record != records.end()){
        if((ret_own_record->second + config::varlitorNum() * varlitorInterval_time) > block_time ) {
            ctrace << " the standby_miner can not seal this time point";
            return false;
        }
    }

    beyond_num = beyond_num - config::minimum_cycle();
    //cnote << " beyond_num:" << beyond_num;

    // find the last_standby_addr for super_minner
    int64_t last_time =0;
    Address last_standby_addr = Address();
    int beyond_standby_num =0;               // out_of standby_rounds
    for(auto const& val: standby_addrs){
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
    //cnote << "last_time:" << last_time << " last_standby_addr:"<<last_standby_addr << " beyond_standby_num:"<<beyond_standby_num;

    bool is_loop_last_standby = false;
    for(auto const& val: standby_addrs){
        if(!last_time) {
            if (beyond_num >=1 ) {
                if (val.m_addr == standby_addr) {
                    //cnote << " not has old record ... will seal..." << standby_addr;
                    return true;
                }
                if (records.count(val.m_addr) && ((records[val.m_addr] + config::varlitorNum() * varlitorInterval_time) > block_time)){
                    --beyond_num ;
                }
            }
        } else{
            if (val.m_addr == standby_addr && beyond_standby_num >=1 ) {
                //cnote<< " has old record ... begins standby will seal..."<< standby_addr;
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