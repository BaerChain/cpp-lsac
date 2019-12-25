#pragma once

#include <string>
#include <libdevcore/CommonJS.h>
#include <libdevcrypto/Common.h>
#include <libdevcore/SHA3.h>
#include "Common.h"

// devcrypto

namespace dev
{

/// Leniently convert string to Public (h512). Accepts integers, "0x" prefixing, non-exact length.
inline Public jsToPublic(std::string const& _s) { return jsToFixed<sizeof(dev::Public)>(_s); }

/// Leniently convert string to Secret (h256). Accepts integers, "0x" prefixing, non-exact length.
inline Secret jsToSecret(std::string const& _s) { h256 d = jsToFixed<sizeof(dev::Secret)>(_s); Secret ret(d); d.ref().cleanse(); return ret; }

/// Leniently convert string to Address (h160). Accepts integers, "0x" prefixing, non-exact length.
inline Address jsToAddress(std::string const& _s) { return brc::toAddress(_s); }

/// Convert u256 into user-readable string. Returns int/hex value of 64 bits int, hex of 160 bits FixedHash. As a fallback try to handle input as h256.
std::string prettyU256(u256 _n, bool _abridged = true);

Address jsToAddressFromNewAddress(std::string const& _ns);
std::string jsToNewAddress(std::string const _s);

std::string to2HashAddress(Address const& _addr);

}


namespace dev
{
namespace brc
{

/// Convert to a block number, a bit like jsToInt, except that it correctly recognises "pending" and "latest".
BlockNumber jsToBlockNumber(std::string const& _js);
BlockNumber jsToBlockNum(std::string const& _js);
uint8_t jsToOrderEnum(std::string const& _js);
int64_t jsToint64(std::string const& _js);
}
}
