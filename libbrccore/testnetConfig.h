#ifndef BAERCHAIN_TESTNETCONFIG_H
#define BAERCHAIN_TESTNETCONFIG_H

#include <string>

namespace dev{

    namespace brc{


        class testnetConfig {
            testnetConfig(){}
            ~testnetConfig(){}

        public:
            static std::string const& getTestnetGenesis();
            static std::string const& getTestnetConfig();
        };
    }
}




#endif //BAERCHAIN_TESTNETCONFIG_H
