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

    //ExchangeFee 45786368616e6765466565
    Address const PdSystemAddress   { "0x00000000000000000045786368616e6765466565"};

    //7265637963656c
    Address const RecycelSystemAddress   { "0x000000000000000000000000007265637963656c"};
    //char [%d %x]

    Address const SysBlockCreateRecordAddress   { "0x000000537973425265636f726441646472657373"};
    Address const SysElectorAddress             { "0x000000000000456c6563746f7241646472657373"};
    Address const SysVarlitorAddress            { "0x000000000067656e657369735661726c69746f72" };
    Address const SysCanlitorAddress            { "0x0000000067656e6573697343616e646964617465" };

}  // namespace dev

