#pragma once

#include <libdevcore/Exceptions.h>

namespace dev
{
namespace crypto
{

/// Rare malfunction of cryptographic functions.
DEV_SIMPLE_EXCEPTION(CryptoException);
DEV_SIMPLE_EXCEPTION(Base58_decode_exception);
DEV_SIMPLE_EXCEPTION(Base58_encode_exception);

}
}
