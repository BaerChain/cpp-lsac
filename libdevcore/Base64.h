#pragma once

#include <string>
#include "FixedHash.h"

namespace dev
{

std::string toBase64(bytesConstRef _in);
bytes fromBase64(std::string const& _in);

}
