#pragma once

#include "State.h"
#include <libdevcore/Common.h>
#include "Transaction.h"
#include "DposVote.h"

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

        enum class ReceivingType : uint8_t
        {
            RReceivingNull = 0,
            RBlockFeeIncome = 1,
            RPdFeeIncome
        };

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
									 std::vector<std::shared_ptr<transationTool::operation>> const& _ops);

			void verifyCancelPendingOrder(ex::exchange_plugin &_exdb, Address _addr, h256 _HashV);
			void verifyCancelPendingOrders(ex::exchange_plugin &_exdb, Address _addr, std::vector<std::shared_ptr<transationTool::operation>> const& _ops);
			void verifyreceivingincome(Address const& _from, std::vector<std::shared_ptr<transationTool::operation>> const& _ops,transationTool::dividendcycle _type, EnvInfo const& _envinfo, DposVote const& _vote);
            void verifyBlockFeeincome(Address const& _from, EnvInfo const& _envinfo, DposVote const& _vote);
            void verifyPdFeeincome(Address const& _from, int64_t _blockNum, DposVote const& _vote);
            bool findAddress(std::map<Address, u256> const& _voteData, std::vector<PollData> const& _pollData);
        private:
            State &m_state;
        };


    }  // namespace brc
}  // namespace dev
