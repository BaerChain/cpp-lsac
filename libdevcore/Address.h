#pragma once

#include "FixedHash.h"

namespace dev
{

/// An BrcdChain address: 20 bytes.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Address = h160;

/// A vector of BrcdChain addresses.
using Addresses = h160s;

/// A hash set of BrcdChain addresses.
using AddressHash = std::unordered_set<h160>;

/// The zero address.
extern Address const ZeroAddress;

/// The last address.
extern Address const MaxAddress;

/// The SYSTEM address.
extern Address const SystemAddress;

/// The VOTE address
extern Address const VoteAddress;

// the systemAddress to save Elector's Address 
extern Address const SysElectorAddress;

extern Address const systemAddress;

extern Address const SysBlockCreateRecordAddress;

extern Address const SysVarlitorAddress;

extern Address const SysCanlitorAddress;

extern Address const PdSystemAddress;

extern Address const SysMinerSnapshotAddress;

extern Address const ExdbSystemAddress;

extern Address const TestbplusAddress;
}

