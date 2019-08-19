//
// Created by friday on 2019/8/19.
//

#ifndef BAERCHAIN_EXDBSTATE_H
#define BAERCHAIN_EXDBSTATE_H

#include "State.h"
#include <libdevcore/Common.h>
#include <libdevcore/Address.h>
#include <libdevcrypto/Common.h>
#include <libbvm/ExtVMFace.h>
#include <libbrccore/config.h>

namespace dev{
    namespace brc{


        class ExdbState {
        public:
            ExdbState(State &_state):m_state(_state){}

            /// insert operation .
            /// \param orders   vector<order>
            /// \param reset    if true , this operation will reset, dont record.
            /// \param throw_exception  if true  throw exception when error.
            /// \return
            ex::result_order insert_operation(const ex::order &od);

            ///
            /// \param os vector transactions id
            /// \param reset   if true, this operation rollback
            /// \return        vector<orders>
            ex::order  cancel_order_by_trxid(const h256 &id);
        private:
            State  &m_state;
        };



    }
}

#endif //BAERCHAIN_EXDBSTATE_H
