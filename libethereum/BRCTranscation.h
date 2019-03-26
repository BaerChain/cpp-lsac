#include "State.h"
#include <libdevcore/Common.h>


namespace dev
{
namespace eth
{
enum TranscationEnum
{
    ENull = 0,
    EBRCTranscation,
    ECookieTranscation,
    ETranscationMax
}

class BRCTranscation
{
public:
    BRCTranscation(State& _state) : m_state(_state) {}
    ~BRCTranscation() {}
    void setState(State& _s) { m_state = _s; }
public:
    bool verifyTranscation(Address const& _form, Address const& _to, size_t _type, size_t _transcationNum);
private:
    State& m_state;
};


}  // namespace eth
}  // namespace dev
