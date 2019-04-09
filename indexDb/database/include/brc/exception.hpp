//
// Created by fri on 2019/4/8.
//

#pragma once

#include <libdevcore/Exceptions.h>

namespace dev{
    DEV_SIMPLE_EXCEPTION(createOrderError);
    DEV_SIMPLE_EXCEPTION(order_type_is_null);
    DEV_SIMPLE_EXCEPTION(remove_object_error);
}
