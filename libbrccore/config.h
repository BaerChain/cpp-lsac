#ifndef BEARCHAIN_CONFIG_H
#define BEARCHAIN_CONFIG_H

#include <libdevcore/Common.h>

namespace dev
{
    namespace brc{


        enum class Votingstage : uint8_t
        {
            VOTE = 0,
            RECEIVINGINCOME = 1,
            ERRORSTAGE = 2
        };


        class config
        {
            public:
                config(){}
                ~config(){}
                static std::pair<uint32_t, Votingstage> getVotingCycle(int64_t _blockNum);
                ///@return varlitor num in chain
                static  u256 varlitorNum() { return 21;}
                ///@return alternate num in chain
                static  u256 standbyNum() { return 30;}
                ///@return Minimum period to enable standby_node if the super_node not to create_block
                static  uint32_t minimum_cycle() { return  3;}
        };
    }
}



#endif //BEARCHAIN_CONFIG_H
