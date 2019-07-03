#pragma once

#include "Common.h"

namespace dev
{

bytes aesDecrypt(bytesConstRef _cipher, std::string const& _password, unsigned _rounds = 2000, bytesConstRef _salt = bytesConstRef());

}
