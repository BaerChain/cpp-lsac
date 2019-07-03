#include "Common.h"

void dev::bacd::SH_DposBadBlock::streamRLPFields(RLPStream& _s) 
{
	SHDPosStatusMsg::streamRLPFields(_s);
	_s << (h520)m_signData;
	_s << m_badBlock;
}

void dev::bacd::SH_DposBadBlock::populate(RLP const& _rlp)
{
	size_t field = 0;
	SHDPosStatusMsg::populate(_rlp);
	try
	{
		m_signData = _rlp[field = 1].toHash<h520>(RLP::VeryStrict);
		m_badBlock = _rlp[field = 2].toBytes();
	}
	catch(Exception const& /*_e*/)
	{
		/*_e << errinfo_name("invalid msg format") << BadFieldError(field, toHex(_rlp[field].data().toBytes()));
		throw;*/
		cwarn << "populate DposDataMsg is error field:  " << field;
	}
}
