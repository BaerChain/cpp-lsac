#pragma once

#include "State.h"
#include <libdevcore/Common.h>
#include <brc/types.hpp>

namespace dev {
    namespace brc {
        enum TranscationEnum {
            ETranscationNull = 0,
            EBRCTranscation,
            ETranscationMax
        };

        enum PendingOrderEnum {
            EPendingOrderNull = 0,
            EBuyBrcPendingOrder,
            ESellBrcPendingOrder,
            EBuyFuelPendingOrder,
            ESellFuelPendingOrder,
            ECancelPendingOrder,
            EPendingOrderMax
        };

		/*Transfer_brc = 1,
			Buy_brc = 1 << 1,
			Buy_cookie = 1 << 2,
			Sell_brc = 1 << 3,
			Sell_cookie = 1 << 4,
			Cancel_pending = 1 << 5,
			Buy_tickets = 1 << 6,
			Sell_tickets = 1 << 7,
			Vote = 1 << 8,
			Cancel_vote = 1 << 9,
			Login_candidata = 1 << 10,
			Logout_candidate = 1 << 11,
			Deploy_contract = 1 << 12,
			Execute_contract = 1 << 13,*/

        class BRCTranscation {
        public:
            BRCTranscation(State &_state) : m_state(_state) {}

            ~BRCTranscation() {}

            void setState(State &_s) { m_state = _s; }

        public:
            void verifyTranscation(
                    Address const &_form, Address const &_to, size_t _type, const u256 &_transcationNum);

            void verifyPendingOrder(Address const &_form, ex::exchange_plugin &_exdb, int64_t _nowTime, ex::order_type _type,
                                    ex::order_token_type _token_type, ex::order_buy_type _buy_type, u256 _pendingOrderNum,
                                    u256 _pendingOrderPrice, u256 _transcationGas = u256(0) ,h256 _pendingOrderHash = h256(0));
			void verifyPendingOrders(Address const& _addr, u256 _total_cost, ex::exchange_plugin& _exdb,
									 int64_t _nowTime, u256 _transcationGas, h256 _pendingOrderHash, 
									 std::vector<std::shared_ptr<transationTool::operation>> const& _ops, uint64_t _authority);

			void verifyCancelPendingOrder(ex::exchange_plugin &_exdb, Address _addr, h256 _HashV);
			void verifyCancelPendingOrders(ex::exchange_plugin &_exdb, Address _addr, std::vector<std::shared_ptr<transationTool::operation>> const& _ops);

		private:
			Authority_type get_authority_type(ex::order_type _type, ex::order_token_type _token_type)const;

        private:
            State &m_state;
        };


    }  // namespace brc
}  // namespace dev
