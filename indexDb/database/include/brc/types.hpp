#pragma once

#include <stdint.h>
namespace dev {
    namespace brc {
        namespace ex {

            typedef int64_t Time_ms;


            enum class order_type : uint8_t {
                null_type = 0,
                sell = 1,
                buy = 2

            };

            enum class order_token_type : uint8_t { 
                BRC = 0,
                FUEL                   // => cook
            };


            enum class order_buy_type : uint8_t {
                all_price = 0,
                only_price
            };
        }
    }
}

