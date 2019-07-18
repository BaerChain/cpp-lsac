#include "config.h"
#include "Exceptions.h"

using namespace dev;
using namespace dev::brc;

std::pair<uint32_t, Votingstage> config::getVotingCycle(int64_t _blockNum)
{
    if(_blockNum >= 0 && _blockNum < 40)
    {
        if(_blockNum >= 0 && _blockNum < 20)
        {
            return std::pair<uint32_t , Votingstage>(2, Votingstage::VOTE);
        }else if(_blockNum >= 20 && _blockNum < 40)
        {
            return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
        }
    }else if(_blockNum >= 40 && _blockNum < 80){
        if(_blockNum >= 40 && _blockNum < 60)
        {
            return std::pair<uint32_t, Votingstage>(3, Votingstage::VOTE);
        }else{
            return std::pair<uint32_t, Votingstage>(3, Votingstage::RECEIVINGINCOME);
        }
    }else if(_blockNum >= 80 && _blockNum < 1000) {
        if (_blockNum >= 80 && _blockNum < 100) {
            return std::pair<uint32_t, Votingstage>(4, Votingstage::VOTE);
        } else {
            return std::pair<uint32_t, Votingstage>(4, Votingstage::RECEIVINGINCOME);
        }
    }
    else{
        //return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
        //BOOST_THROW_EXCEPTION(getVotingCycleFailed() << errinfo_comment(std::string("getVotingCycle error : Current time point is not in the voting period")));
        return std::pair<uint32_t, Votingstage>(-1, Votingstage::ERRORSTAGE);
    }
}