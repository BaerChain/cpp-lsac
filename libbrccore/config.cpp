#include "config.h"
#include "Exceptions.h"

using namespace dev;
using namespace dev::brc;

std::pair<uint32_t, Votingstage> config::getVotingCycle(int64_t _blockNum)
{
    if(_blockNum >= 0 && _blockNum < 1700000)
    {
        if(_blockNum >= 0 && _blockNum < 1000000)
        {
            return std::pair<uint32_t , Votingstage>(2, Votingstage::VOTE);
        }else if(_blockNum >= 1000000 && _blockNum < 1700000)
        {
            return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
        }
    }
    //return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
    //BOOST_THROW_EXCEPTION(getVotingCycleFailed() << errinfo_comment(std::string("getVotingCycle error : Current time point is not in the voting period")));
    return std::pair<uint32_t, Votingstage>(-1, Votingstage::ERRORSTAGE);

}

uint32_t config::varlitorNum() { return 21;}

uint32_t config::standbyNum() { return 30;}

uint32_t config::minimum_cycle() { return  3;}