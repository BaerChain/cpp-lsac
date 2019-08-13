#include "config.h"
#include "Exceptions.h"

using namespace dev;
using namespace dev::brc;

std::pair<uint32_t, Votingstage> config::getVotingCycle(int64_t _blockNum)
{
    if(_blockNum >= 0 && _blockNum < 80){
        if(_blockNum >= 0 && _blockNum < 60)
        {
            return std::pair<uint32_t , Votingstage>(2, Votingstage::RECEIVINGINCOME);
        }else if(_blockNum >= 60 && _blockNum < 80)
        {
            return std::pair<uint32_t, Votingstage>(3, Votingstage::VOTE);
        }

    } else if(_blockNum >= 80 && _blockNum < 140){
        if(_blockNum >= 80 && _blockNum < 120)
        {
            return std::pair<uint32_t , Votingstage>(2, Votingstage::RECEIVINGINCOME);
        }else if(_blockNum >= 120 && _blockNum < 140)
        {
            return std::pair<uint32_t, Votingstage>(3, Votingstage::VOTE);
        }

    }
    //return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
    //BOOST_THROW_EXCEPTION(getVotingCycleFailed() << errinfo_comment(std::string("getVotingCycle error : Current time point is not in the voting period")));
    return std::pair<uint32_t, Votingstage>(-1, Votingstage::ERRORSTAGE);

}

uint32_t config::varlitorNum() { return config::getInstance().varlitor_num;}

uint32_t config::standbyNum() {  return config::getInstance().standby_num;}

uint32_t config::minimum_cycle() { return config::getInstance().min_cycel;}

uint32_t config::max_message_num() { return 50;}

u256 config::getvoteRound(dev::u256 _numberofrounds)
{
    if(_numberofrounds == 0)
    {
        return 2;
    }
    return _numberofrounds;
}
static std::string const c_genesisInfoMainNetwork = std::string() +
R"E(
{
  "sealEngine":"SHDpos",
  "params": {
    "accountStartNonce": "0x00",
    "networkID": "0x0",
    "chainID": "0x1",
    "maximumExtraDataSize": "0x20",
    "tieBreakingGas": false,
    "minGasLimit": "0x1388",
    "maxGasLimit": "7fffffffffffffffffffff",
    "minimumDifficulty":"0x14",
    "dposEpochInterval": 0,
    "dposVarlitorInterval":1000,
    "dposBlockInterval": 1000
  },
  "genesis":{
    "nonce": "0x0000000000000042",
    "difficulty": "0x200",
    "mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
    "author": "0x0000000000000000000000000000000000000000",
    "timestamp": "0x00",
    "parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
    "extraData": "0x11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa",
    "gasLimit": "0x7ffffffffffffffffffff",
    "sign":"0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
  },
  "accounts":{
    "000000000067656e657369735661726c69746f72":{
        "genesisVarlitor": [
            "4d51ba213bad0f7ed97d336b2c00dc76576aac98",
            "8870f2e94d485ae78e70269a730b34c30132d88d",
            "19472617f1b685dd89b05d4c9eadb742da966a3c",
            "687ec522068acf768a6d3d831c39fadb674b39da",
            "49d233b6815dc0970458036cd1b1f9b0aaeb1e4f",
            "239d5368178b761b4b2f167a908e9797620fb934",
            "adac92008f03107e11cec579b66a26b2b0f31686",
            "a105a2971e22df8678c1977e29ccc2b176acec9d",
            "bd0f16716a14a758b71386aee788aca8f5bbb263",
            "c815dc68af24de193269fef5901eaaf23919ccf1",
            "643dddd85490ab2027485db726f789b26ea15966",
            "b24219bfb32cc17cf85f0ee5cbead3d7907cd77d",
            "136383aeeaf7b061843c788d3b37b65ebb53fa6d",
            "35e90dff4f118e2bb724b046ce041fb0e1ccdd1e",
            "455081db530d645f6a9c8baffd1580e965216e81",
            "b7e8d387ea4783afa73fdbce7feb78636d827fe2",
            "a3e14e35079fe44267bdd94babbb50dc2a4fe448",
            "adc2ec205e626826ac2b3ccabf2c7595b1a2b941",
            "29ba89c1e7993542eb759a060749bf3269526ecd",
            "cd77a1540e2e93278a89e0ded3b027d7dd6935ef",
            "0c40635a7e0e12958d8ebd0e4e8dca846f48e293"
            ]
    },
    "0000000067656e6573697343616e646964617465":{
        "genesisVarlitor": [
            "42daa2ba4f1e521b0e36fc486aa67ff335071c11",
            "5be003232b41ae83e23801ef3794b30e5c19e27e",
            "910ab9bd72f1fc270e7793402936dcbd7aff604d",
            "cb2cb2608f63b123d8bd2ffe223ecaeff78d6ead",
            "783a36b30543ab1d372bc2e75785ddfaf0c6100e",
            "8f6b2ea48cdc75c7a242e3a9f28b6bd012e28d12",
            "ba1dde2d2ab18d671ac5978329306c64f1c9b9ce",
            "b55156b074f43358366cd35e37399518fc6e24ed",
            "f69db1cc9033f8b9aeae7475291f0aa38512109b",
            "5766268cde20efbb2e6749d098472c7a933cdc9e",
            "566c40c134f26c79e7237cdeac9e0b90a9b38f48",
            "a7f38b517edc7ad5fb0ead944c033d3862b48eea",
            "abd147e73176b1759a9e004f66ee24e46fc964f9",
            "a32d4138fe723c7107c2da315fd18c7f3d1ea20b",
            "997d7d94b715c37d599d6312dd5257fb652cd444",
            "b512b990a81d7d86131135688280e9a2a242a067",
            "db8afb72a65304e06862498f8b49650743965972",
            "9fd975135af939121e9a9261821e4a4cbd2565a1",
            "84bcca74c4bc1cb66cc57550fe2ee9d98f8da8c1",
            "1fe79dc07f1fb4be5e5c3a1cffd1cf3108ac0e35",
            "c36ebae8002b7ceeade7e20a17fffb8e062fe236",
            "c7c2b38460c41a89b885ac8d1a9dc4f4d9fddf2f",
            "913af8f6e9f60b2bce219c4719d9cc3320b9d564",
            "325914895aeacf420dc7774c9f29e2430f3e0c79",
            "ee3d85bc5dfc24968d845bbc7106b38029f53495",
            "1195b47186cd5da8423a32d802bdfd0a430dd7ff",
            "81e69e5f8a084d68c502a6cbb80441d546c1ed14",
            "e7733d4288baf97848b05ff5be7016fa5ec24c57",
            "97223efabcd242c1e720f749f3148bd0f9274d1c",
            "42f5407a82359fb15125cff542c0ec2f4baacb1f"
        ]
    },
    "000000000000000000000042616572436861696e":{
        "currency":{
                "cookies": "0",
                "fcookie": "2900000000000000"
        }
    },
    "e79fead329b69540142fabd881099c04424cc49f":{
        "currency":{
                "cookies": "2900000000000000",
                "brc": "2900000000000000"
        }
    },
    "db1874c67cf3628690c795f0553e0e2904b02ffd":{
        "currency":{
                "cookies": "2900000000000000",
                "brc": "2900000000000000"
        }
    },
    "c3eb4d16a99e0a04de3242729a759a45809d2628":{
        "currency":{
                "cookies": "2900000000000000",
                "brc": "2900000000000000"
        }
    }
}
}
)E";

std::string const &config::genesis_info(ChainNetWork chain_type){
    switch (chain_type){
        case ChainNetWork ::MainNetwork:
            return  c_genesisInfoMainNetwork;
        default:
            throw std::invalid_argument("Invalid network value");
    }
}


