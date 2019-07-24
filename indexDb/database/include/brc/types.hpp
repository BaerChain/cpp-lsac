#pragma once

#include <stdint.h>
#include <string>
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

            template <typename T>
            std::string enum_to_string(const T &t);

            template <>
            std::string enum_to_string<order_buy_type>(const order_buy_type &t);

            template <>
            std::string enum_to_string<order_token_type>(const order_token_type &t);

            template <>
            std::string enum_to_string<order_type>(const order_type &t);

        }
    }
}

