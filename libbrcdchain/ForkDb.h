//
// Created by friday on 23/05/2019.
//

#pragma once

#include <libdevcore/OverlayDB.h>
#include <brc/exchangeOrder.hpp>

namespace dev{
    namespace brc{






        /*  @brief this class process many block with same block number.
         *         when push new block, this class will consider to switch chain or remove chain.
         *         this class only record block , and switch chain.
         *  example:
         *      A6  A7  (main chain)  ... A17(now, push this block)
         *  5   B6  B7                ...    (this chain will be removed)
         *      C6  C7                ...    (this chain will be removed)
         *
         *
         *  example2:
         *
         *      A6  A7  (main chain)                =>       A6  A7
         *  5   B6  B7                              =>   5   B6  B7
         *      C6  C7  C8(now push this block)     =>       C6  C7 C8 (this chain will be main chain, state_db and ex_db will switch to this state.so configure two database state is equal.)
         *
         *
         * */
        class fork_database{
        public:
            fork_database(uint32_t config_number);




        private:
            uint32_t max_numbers;
        };





    }
}