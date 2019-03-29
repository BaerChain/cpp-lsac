#pragma once

#include <libdevcore/TrieDB.h>

namespace dev
{
namespace brc
{
#if BRC_FATDB
template <class KeyType, class DB>
using SecureTrieDB = SpecificTrieDB<FatGenericTrieDB<DB>, KeyType>;
#else
template <class KeyType, class DB>
using SecureTrieDB = SpecificTrieDB<HashedGenericTrieDB<DB>, KeyType>;
#endif

}  // namespace brc
}  // namespace dev
