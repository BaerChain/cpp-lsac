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
    bool verifyPendingOrder(Address const& _form, exchange_plugin const& _exdb, int64_t _nowTime, size_t _type, size_t _token_type, size_t _buy_type, size_t _pendingOrderNum, size_t& _pendingOrderPrice, h256 _pendingOrderHash = 0);

    private:
        State& m_state;
    };


    }  // namespace brc
}  // namespace dev
