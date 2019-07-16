#include "config.h"
#include "Exceptions.h"

using namespace dev;
using namespace dev::brc;

std::pair<uint32_t, Votingstage> config::getVotingCycle(int64_t _blockNum)
{
    if(_blockNum >= 0 && _blockNum < 100)
    {
        if(_blockNum >= 0 && _blockNum < 20)
        {
            return std::pair<uint32_t , Votingstage>(1, Votingstage::VOTE);
        }else if(_blockNum >= 20 && _blockNum < 100)
        {
            return std::pair<uint32_t, Votingstage>(1, Votingstage::RECEIVINGINCOME);
        }
    }else if(_blockNum >= 100 && _blockNum < 10000){
        if(_blockNum >= 100 && _blockNum < 200)
        {
            return std::pair<uint32_t, Votingstage>(2, Votingstage::VOTE);
        }else{
            return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
        }
    }else{
        //return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
        //BOOST_THROW_EXCEPTION(getVotingCycleFailed() << errinfo_comment(std::string("getVotingCycle error : Current time point is not in the voting period")));
        return std::pair<uint32_t, Votingstage>(-1, Votingstage::ERRORSTAGE);
    }
}