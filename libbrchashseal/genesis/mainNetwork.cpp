#include "../GenesisInfo.h"

static dev::h256 const c_genesisStateRootMainNetwork("d7f8974fb5ac78d9ac099b9ad5018bedc2ce0a72dad1827a1709da30580f0544");
static std::string const c_genesisInfoMainNetwork = std::string() +
R"E(
{
	"sealEngine":"SHDpos",
  "params": {
    "accountStartNonce": "0x00",
    "networkID": "0x0",
    "chainID": "0x0",
    "maximumExtraDataSize": "0x20",
    "tieBreakingGas": false,
    "minGasLimit": "0x1388",
    "maxGasLimit": "7fffffffffffffffffffff",
    "dposEpochInterval": 0,
    "dposVarlitorInterval":1000,
    "dposBlockInterval": 1000
  },
  "genesis": {
    "nonce": "0x0000000000000042",
    "difficulty": "0x200",
    "mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
    "author": "0x0000000000000000000000000000000000000000",
    "timestamp": "0x00",
    "parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
    "extraData": "0x11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa",
    "gasLimit": "0x0xffffffffffffffffffffffffffffff",
    "sign":"0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
  },
  "accounts":{
    "000000000067656e657369735661726c69746f72":{
        "genesisVarlitor": [
            "042610e447c94ff0824b7aa89fab123930edc805",
            "e523e7c59a0725afd08bc9751c89eed6f8e16dec"
            ]
    },
    "0000000067656e6573697343616e646964617465":{
        "genesisVarlitor": []
    }
}
}
)E";
