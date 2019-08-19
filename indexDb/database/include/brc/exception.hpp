
#pragma once

#include <libdevcore/Exceptions.h>

namespace dev{
    DEV_SIMPLE_EXCEPTION(createOrderError);
    DEV_SIMPLE_EXCEPTION(order_type_is_null);
    DEV_SIMPLE_EXCEPTION(remove_object_error);
    DEV_SIMPLE_EXCEPTION(get_db_instance_error);
    DEV_SIMPLE_EXCEPTION(all_price_operation_error);
    DEV_SIMPLE_EXCEPTION(order_price_num_equal_zero_error);
    DEV_SIMPLE_EXCEPTION(find_order_trxid_error);
    DEV_SIMPLE_EXCEPTION(get_result_order_error);
}
