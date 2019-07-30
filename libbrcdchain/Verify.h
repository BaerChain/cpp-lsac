#pragma once

#include <State.h>
#include <libdevcore/Address.h>


namespace dev{
    namespace brc{
        class Verify{
        public:
            Verify(State const& state): m_state(state){}

            ///verify standby_node to create block
            ///@return true if can create
            bool  verify_standby(int64_t block_time, Address const& standby_addr, size_t varlitorInterval_time) const;

        private:
            State const&      m_state;
        };
    }
}