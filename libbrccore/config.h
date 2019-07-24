#ifndef BEARCHAIN_CONFIG_H
#define BEARCHAIN_CONFIG_H

#include <libdevcore/Common.h>

#define PRICEPRECISION 100000000
#define MATCHINGFEERATIO 2000
#define SELLCOOKIE 100000000
#define BUYCOOKIE 99000000



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
        };
    }
}



#endif //BEARCHAIN_CONFIG_H
