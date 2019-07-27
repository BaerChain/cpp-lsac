
#include <brc/types.hpp>

namespace dev {
    namespace brc {
        namespace ex {

            template <typename T>
            std::string enum_to_string(const T &t){
                return "null";
            }

            template <>
            std::string enum_to_string<order_buy_type>(const order_buy_type &t){
                if(t == order_buy_type::all_price){
                    return "all_price";
                }
                else if(t == order_buy_type::only_price){
                    return "only_price";
                }
                return "null price";
            }

            template <>
            std::string enum_to_string<order_token_type>(const order_token_type &t){
                if(t == order_token_type::BRC){
                    return "BRC";
                } else if(t == order_token_type::FUEL){
                    return "FUEL";
                }

                return "null_token_type";
            }

            template <>
            std::string enum_to_string<order_type>(const order_type &t){
                if(t == order_type::buy){
                    return "buy";
                } else if(t == order_type::sell){
                    return "sell";
                }
                return "null_sell_buy";
            }

        }
    }
}