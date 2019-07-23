#pragma once

#include <libdevcore/Common.h>

namespace dev
{
namespace crypto
{

std::pair<bool, bytes> alt_bn128_pairing_product(bytesConstRef _in);
std::pair<bool, bytes> alt_bn128_G1_add(bytesConstRef _in);
std::pair<bool, bytes> alt_bn128_G1_mul(bytesConstRef _in);

}
}
