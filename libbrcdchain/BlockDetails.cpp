#include "BlockDetails.h"

#include <libdevcore/Common.h>
using namespace std;
using namespace dev;
using namespace dev::brc;

BlockDetails::BlockDetails(RLP const& _r)
{
	number = _r[0].toInt<unsigned>();
	totalDifficulty = _r[1].toInt<u256>();
	parent = _r[2].toHash<h256>();
	children = _r[3].toVector<h256>();
	size = _r.size();
}

bytes BlockDetails::rlp() const
{
	auto ret = rlpList(number, totalDifficulty, parent, children);
	size = ret.size();
	return ret;
}
