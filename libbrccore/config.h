#ifndef BEARCHAIN_CONFIG_H
#define BEARCHAIN_CONFIG_H

#include <libdevcore/Common.h>

#define PRICEPRECISION 100000000
#define MATCHINGFEERATIO 2000
#define SELLCOOKIE 100000000
#define BUYCOOKIE 99990000



namespace dev
{
    namespace brc{


        enum class Votingstage : uint8_t
        {
            VOTE = 0,
            RECEIVINGINCOME = 1,
            ERRORSTAGE = 2
        };

        enum class ChainNetWork{
            ///< Normal Olympic chain.
            // Olympic = 0,
            /// main_network chain for open.
            MainNetwork = 1,
        };

        //extern std::string const c_genesisInfoMainNetwork;


        class config
        {
            public:
                static std::pair<uint32_t, Votingstage> getVotingCycle(int64_t _blockNum);

                static const config getInstance(uint32_t _varlitor_num = 21, uint32_t _standby_num =30, uint32_t _min_cycle =3){
                    static config instance;
                    if(!instance.is_init){
                        instance.varlitor_num = _varlitor_num;
                        instance.standby_num =_standby_num;
                        instance.min_cycel = _min_cycle;
                        instance.is_init = true;
                    }
                    return instance;
                }

                ///@return varlitor num in chain
                static  uint32_t varlitorNum();
                ///@return alternate num in chain
                static  uint32_t standbyNum();
                ///@return Minimum period to enable standby_node if the super_node not to create_block
                static  uint32_t minimum_cycle();
                static  uint32_t minner_rank_num() { return  7;}
                ///@return the max num of rpc_interdace message
                static  uint32_t max_message_num();

                static std::string const& genesis_info(ChainNetWork chain_type);

                static u256 getvoteRound(u256 _numberofrounds);
        private:
            config(){}
            //~config(){}
            bool is_init = false;
            uint32_t varlitor_num =21;
            uint32_t standby_num = 30;
            uint32_t min_cycel = 3;
        };
    }
}



#endif //BEARCHAIN_CONFIG_H
