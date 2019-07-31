#pragma once

#include <State.h>
#include <libdevcore/Address.h>


namespace dev{
    namespace brc{
        class Verify{
        public:
            Verify(){}

            ///verify standby_node to create block
            ///@return true if can create
            bool  verify_standby(State const& state, int64_t block_time, Address const& standby_addr, size_t varlitorInterval_time) const;

        };
    }
}