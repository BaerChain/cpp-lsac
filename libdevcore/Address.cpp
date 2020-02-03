#include "Address.h"

namespace dev
{
    Address const ZeroAddress;
    Address const MaxAddress        {"0xffffffffffffffffffffffffffffffffffffffff"};
    Address const SystemAddress     {"0xfffffffffffffffffffffffffffffffffffffffe"};
    Address const VoteAddress       { "0x00000000000000000000000000000000766f7465"};
    Address const systemAddress     { "0x000000000000000000000042616572436861696e"};

    //ExchangeFee 45786368616e6765466565
    Address const PdSystemAddress   { "0x00000000000000000045786368616e6765466565"};

    //7265637963656c
    Address const RecycelSystemAddress   { "0x000000000000000000000000007265637963656c"};
    //char [%d %x]

    Address const ExdbSystemAddress             { "0x0000000000000000006578646241646472657373"};

    Address const SysBlockCreateRecordAddress   { "0x000000537973425265636f726441646472657373"};
    Address const SysElectorAddress             { "0x000000000000456c6563746f7241646472657373"};
    Address const SysVarlitorAddress            { "0x000000000067656e657369735661726c69746f72" };
    Address const SysCanlitorAddress            { "0x0000000067656e6573697343616e646964617465" };
    Address const SysMinerSnapshotAddress       { "0x00000000005379734d696e657241646472657373" };

    //test code 
    Address const TestbplusAddress              { "0x0000000062706c75737465737441646472657373"};
    //new orderAddress
    Address const BuyExchangeAddress            { "0x000042757945786368616e676541646472657373"};
    Address const SellExchangeAddress           { "0x0053656c6c45786368616e676541646472657373"};
}  // namespace dev

