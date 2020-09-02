#ifndef BAERCHAIN_GENESIS_H
#define BAERCHAIN_GENESIS_H

#include <libdevcore/Common.h>
#include <libdevcore/Address.h>
namespace dev
{  
    namespace brc{
        
        enum class ChainNetWork{
            ///< Normal Olympic chain.
            // Olympic = 0,
            /// main_network chain for open.
            MainNetwork = 1,
        };

        class genesis{
            public:     
            static std::string const& genesis_info(ChainNetWork chain_type);
            // static std::vector<Address> const& getGenesisAddr();
        };
    } 
    
} // namespace dev




#endif //BAERCHAIN_GENESIS_H