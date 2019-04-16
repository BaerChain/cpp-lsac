#pragma once
#include "State.h"
#include <libdevcore/Common.h>


    namespace dev
{
    namespace brc
    {
    enum TranscationEnum
    {
        ETranscationNull = 0,
        EBRCTranscation,
		EFBRCFreezeTranscation,
		EFBRCUnFreezeTranscation,
        ECookieTranscation,
        ETranscationMax
    };

enum PendingOrderEnum
{
    EPendingOrderNull = 0,
    EBuyBrcPendingOrder,
    ESellBrcPendingOrder,
    EBuyFuelPendingOrder,
    ESellFuelPendingOrder,
	ECancelPendingOrder,
    EPendingOrderMax
};

class BRCTranscation
{
public:
    BRCTranscation(State& _state) : m_state(_state) {}
    ~BRCTranscation() {}
    void setState(State& _s) { m_state = _s; }

public:
    bool verifyTranscation(
        Address const& _form, Address const& _to, size_t _type, size_t _transcationNum);
    bool verifyPendingOrder(Address const& _form, ex::exchange_plugin& _exdb, int64_t _nowTime, uint8_t _type, uint8_t _token_type, uint8_t _buy_type, u256 _pendingOrderNum, u256 _pendingOrderPrice, h256 _pendingOrderHash = h256(0));

	bool verifyCancelPendingOrder(std::vector<h256> _HashV);
    private:
        State& m_state;
    };


    }  // namespace brc
}  // namespace dev
