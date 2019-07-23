#pragma once

#include "libdevcore/FixedHash.h"
#include "libdevcore/vector_ref.h"

namespace dev
{

h256 sha256(bytesConstRef _input) noexcept;

h160 ripemd160(bytesConstRef _input);

}
