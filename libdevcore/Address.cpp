#include "Address.h"

namespace dev
{
    Address const ZeroAddress;
    Address const MaxAddress        {"0xffffffffffffffffffffffffffffffffffffffff"};
    Address const SystemAddress     {"0xfffffffffffffffffffffffffffffffffffffffe"};
    Address const VoteAddress       { "0x00000000000000000000000000000000766f7465"};
    Address const ElectorAddress    { "0x000000000000456c6563746f7241646472657373"};
    Address const VarlitorAddress   { "0x00000000005661726c69746f7241646472657373"};
    Address const systemAddress     { "0x000000000000000000000042616572436861696e"};
    Address const PdSystemAddress   { "0x13c014cf9d848081b95a0a2b3372905c0b57b6d8"};
    //char [%d %x]
    //B [66 42]
    //a [97 61]
    //e [101 65]
    //r [114 72]
    //C [67 43]
    //h [104 68]
    //a [97 61]
    //i [105 69]
    //n [110 6e]

    Address const SysBolckCreateRecordAddress   { "0x000000537973425265636f726441646472657373"};
    Address const SysElectorAddress             { "0x000000000000456c6563746f7241646472657373"};
    Address const SysVarlitorAddress            { "0x000000000067656e657369735661726c69746f72" };
    Address const SysCanlitorAddress            { "0x0000000067656e6573697343616e646964617465" };

}  // namespace dev

