//
// Created by friday on 2019/8/19.
//

#include "ExdbState.h"



namespace dev{
    namespace brc{


        ex::result_order ExdbState::insert_operation(const ex::order &od) {
            return ex::result_order();
        }



        ex::order ExdbState::cancel_order_by_trxid(const dev::h256 &id) {


            return  ex::order();
        }



    }
}