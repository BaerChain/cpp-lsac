#include "config.h"
#include "Exceptions.h"

using namespace dev;
using namespace dev::brc;

std::pair<uint32_t, Votingstage> config::getVotingCycle(int64_t _blockNum)
{
    if(_blockNum >= 0 && _blockNum < 10000)
    {
        if(_blockNum >= 0 && _blockNum < 5000)
        {
            return std::pair<uint32_t , Votingstage>(2, Votingstage::VOTE);
        }else if(_blockNum >= 5000 && _blockNum < 10000)
        {
            return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
        }
    }else if(_blockNum >= 10000 && _blockNum < 30000){
        if(_blockNum >= 10000 && _blockNum < 20000)
        {
            return std::pair<uint32_t, Votingstage>(3, Votingstage::VOTE);
        }else{
            return std::pair<uint32_t, Votingstage>(3, Votingstage::RECEIVINGINCOME);
        }
    }else if(_blockNum >= 30000 && _blockNum < 100000) {
        if (_blockNum >= 30000 && _blockNum < 100000) {
            return std::pair<uint32_t, Votingstage>(4, Votingstage::VOTE);
        } else {
            return std::pair<uint32_t, Votingstage>(4, Votingstage::RECEIVINGINCOME);
        }
    }
    //return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
    //BOOST_THROW_EXCEPTION(getVotingCycleFailed() << errinfo_comment(std::string("getVotingCycle error : Current time point is not in the voting period")));
    return std::pair<uint32_t, Votingstage>(-1, Votingstage::ERRORSTAGE);

}

uint32_t config::varlitorNum() { return 21;}

uint32_t config::standbyNum() { return 30;}

uint32_t config::minimum_cycle() { return  3;}