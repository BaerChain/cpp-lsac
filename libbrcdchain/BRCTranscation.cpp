#include "BRCTranscation.h"

bool dev::brc::BRCTranscation::verifyTranscation(
    Address const& _form, Address const& _to, size_t _type, size_t _transcationNum)
{
    if (_type == dev::brc::TranscationEnum::ETranscationNull ||
        _type == dev::brc::TranscationEnum::ETranscationMax ||
        _transcationNum == 0)
    {
        return false;
    }

    if (_type == dev::brc::TranscationEnum::EBRCTranscation)
    {
        if (_form == _to)
        {
            return false;
        }
        if (_transcationNum > (size_t)m_state.BRC(_form))
        {
            return false;
        }
        return true;
    }
    else if (_type == dev::brc::TranscationEnum::ECookieTranscation)
    {
        if (_form == _to || _to != Address() || _transcationNum == 0)
        {
            return false;
        }
        return true;

    }

    return false;
}
