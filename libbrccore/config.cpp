#include "config.h"
#include "Exceptions.h"
#include "Genesis.h"
#include <json/json.h>
#include <libdevcore/Log.h>


using namespace dev;
using namespace dev::brc;

std::pair<uint32_t, Votingstage> config::getVotingCycle(int64_t _blockNum)
{
    uint32_t one_year = 0;
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        one_year = 31536000 * 5;
    }
    else if (config::config::getInstance().m_chainId == TESTCHAINID)
    {
        one_year = 31536000;
    }
    else
    {
        one_year = 31536000;
    }

    const uint32_t one_month = 60 * 60 * 24 * 30;
    if (_blockNum == 0)
        return std::pair<uint32_t, Votingstage>(1, Votingstage::VOTE);
    if (_blockNum > 0 && _blockNum < one_year)
    {
        if (_blockNum > 0 && _blockNum < one_year - one_month)
        {
            return std::pair<uint32_t, Votingstage>(2, Votingstage::RECEIVINGINCOME);
        }
        else if ((_blockNum >= (one_year - one_month)) && _blockNum < one_year)
        {
            return std::pair<uint32_t, Votingstage>(2, Votingstage::VOTE);
        }
    }


    return std::pair<uint32_t, Votingstage>(3, Votingstage::RECEIVINGINCOME);
}

uint32_t config::varlitorNum()
{
    return config::getInstance().varlitor_num;
}

uint32_t config::standbyNum()
{
    return config::getInstance().standby_num;
}

uint32_t config::minimum_cycle()
{
    return config::getInstance().min_cycel;
}

int config::chainId()
{
    return config::getInstance().m_chainId;
}

uint32_t config::max_message_num()
{
    return 50;
}

u256 config::getvoteRound(dev::u256 _numberofrounds)
{
    if (_numberofrounds == 0)
    {
        return 2;
    }
    return _numberofrounds;
}

std::pair<bool, ChangeMiner> config::getChainMiner(int64_t height)
{
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        if (height == 884102)
        {
            return std::make_pair(true, ChangeMiner("0xfe1986a69f6a73ad1050e48b9f9809e1e399b6a2",
                                            "0xa71e748cfe35fb519a85eb277b698e2d5659a614", 500));
        }
    }
    return std::make_pair(false, ChangeMiner());
}

int64_t config::changeVoteHeight()
{
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        return 884100;
    }
    else if (config::config::getInstance().m_chainId == TESTCHAINID)
    {
        return 884100;
    }
    return 2;
}

int64_t config::replaceMinerHeight()
{
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        return 10681001;
    }
    else if (config::config::getInstance().m_chainId == TESTCHAINID)
    {
        return 8759577;
    }
    return 2;
}

int64_t config::initSysAddressHeight()
{
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        return 100;
    }
    else if (config::config::getInstance().m_chainId == TESTCHAINID)
    {
        return 100;
    }
    return 100;
}

int64_t config::newChangeHeight()
{
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        return 10681000;
    }
    else if (config::config::getInstance().m_chainId == TESTCHAINID)
    {
        return 8759580;
    }
    return 2;
}

int64_t config::gasPriceHeight()
{
    // TODO config height
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        return 23814700UL;
    }
    else if (config::config::getInstance().m_chainId == TESTCHAINID)
    {
        return 19650000;
    }
    return 3;
}

u256 config::initialGasPrice()
{
    return 30;
}

int64_t config::autoExHeight()
{
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        return 14524000UL;
    }
    else if (config::config::getInstance().m_chainId == TESTCHAINID)
    {
        return 14524000;
    }
    return 4;
}

int64_t config::autoExTestNetHeight()
{
    return 4072941;
}

int64_t config::newBifurcationBvmHeight() {
	if(config::config::getInstance().m_chainId == MAINCHAINID){
        return 14524000UL;
    }
    else if(config::config::getInstance().m_chainId == TESTCHAINID){
       return 14524000;
    }
	return 10;
}

int64_t config::dividendHeight()
{
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        return 29875800;
    }else if (config::config::getInstance().m_chainId == TESTCHAINID) {
		return INT64_MAX;
	}else
    {
        return 10;
    }
}

int64_t config::cancelAutoPendingOrderHeight()
{
    if (config::config::getInstance().m_chainId == MAINCHAINID)
    {
        return 29875700;
    }
    else
    {
        return 15;
    }
}

std::set<Address> config::getGenesisAddr()
{
    std::string _genesis = genesis::genesis_info(ChainNetWork ::MainNetwork);
    std::set<Address> _ordinaryAddr;
    Json::Value _genesisJson;
    Json::Reader _read;
    if (_read.parse(_genesis, _genesisJson))
    {
        if (!_genesisJson["accounts"].empty())
        {
            Json::Value _accountJson = _genesisJson["accounts"];
            std::vector<std::string> _memberName = _accountJson.getMemberNames();

            for (auto const& _accountAddr : _memberName)
            {
                if (Address(_accountAddr) == SysVarlitorAddress)
                {
                    continue;
                }
                else if (Address(_accountAddr) == SysCanlitorAddress)
                {
                    continue;
                }
                else
                {
                    if (!_accountJson[_accountAddr].empty())
                    {
                        Json::Value _accountInfo = _accountJson[_accountAddr];
                        if (!_accountInfo["vote"].empty())
                        {
                            _ordinaryAddr.insert(Address(_accountAddr));
                        }
                    }
                }
            }
        }
    }
    return _ordinaryAddr;
}

std::map<Address, u256> config::getMasterNodeInfo()
{
    std::map<Address, u256> _masterNode = {
        {Address("0x6ad984cc22f0fbed8cfc5b41ea6855ac704bd59e"), u256("1572757713040000")},
        {Address("0xd5efe84a060b41f17083139a4eafc770de094079"), u256("758983173400000")},
        {Address("0x0c49741d907aa2ec579c454c4e2ba58c756e6829"), u256("558465657200000")},
        {Address("0x0fb86cda6c29430b7f883d5bd7173b37ca4c8582"), u256("551835279000000")},
        {Address("0x26e0e85302db7d8907ea79c4e16a74c3c6a6b55a"), u256("544597616000000")},
        {Address("0x1436e9b7069339f57d7c075d32341daf5cb744e5"), u256("543456430000000")},
        {Address("0x944bca7e6bf23b1b72d71196ad041ecdeffb188f"), u256("543444615000000")},
        {Address("0xcd7401e6cb6864d3c9c247992518c0a5daf61c12"), u256("487772121800000")},
        {Address("0x8a6e14d9a8e2fdf8971c3996f14cb66f949f4970"), u256("376080959600000")},
        {Address("0x067a972a98851f00313ce5de08e3a4c8f67f74d2"), u256("369756718000000")},
        {Address("0x0252fcd7cd7c154e6028fb501f8e630848d26c86"), u256("366843431600000")},
        {Address("0x7dbd4f7878e844b908016d7eb8f68a57d80cec0a"), u256("348590757600000")},
        {Address("0x662c52ee941a1112bde8f7477c52d2d42c4808de"), u256("344939304000000")},
        {Address("0xcfe1985086fb87a364f9f6344674275befd2a510"), u256("305045580400000")},
        {Address("0x6d0e7d50e7b1e468f7a97e0429f97b8465585935"), u256("287848762800000")},
        {Address("0x502381b4c9a727278fecd9a1cd8ba208bb63ad4a"), u256("285230454600000")},
        {Address("0x8305a7c4381b35592aadca063704c5413aca7b92"), u256("283634527000000")},
        {Address("0x20b9d014d5926c8aa758b766ff3699ac2480d5e1"), u256("283225469800000")},
        {Address("0x15643fc8ead0f9aa46d50d48d722c686a303b1c0"), u256("280702993200000")},
        {Address("0xf78b86783c5b39d81b04a61e2c90fb5bfdbd047b"), u256("279488423800000")},
        {Address("0xa71e748cfe35fb519a85eb277b698e2d5659a614"), u256("279227052400000")},
        {Address("0x150452f690d2424cf366062d9aabafec075c4e77"), u256("446112102600000")},
        {Address("0x37111fb7174bafc173af4e3fd989423ee04da34a"), u256("372141800200000")},
        {Address("0x84ae0404cbd02d5f339b3de67f0c2f48437c4b7f"), u256("291060972200000")},
        {Address("0xf76529f84d8ff60ba90c8a3cbd43792f76cddb00"), u256("279523676600000")},
        {Address("0xc670beb4f50f58e22e76705ca1f1e35cb5b52791"), u256("240513991800000")},
        {Address("0x0caab777158abc1985a1cc0c878e55f888c72f74"), u256("239237727200000")},
        {Address("0xcdbbfc1b67a67723fdb0330f6d2ec905950ad35b"), u256("216984794000000")},
        {Address("0x71544ebeb05af0f4430ac89e17ea540dfe16a68f"), u256("176099590320000")},
        {Address("0xf9328355833e277e6d01b3f42e60f5740577ad05"), u256("152116424740000")},
        {Address("0x273e65c98ad345d6de3b5b2acc0a2b86b5859c8c"), u256("140812610320000")},
        {Address("0x03d283a848fadc4361ac1ea568b2ab76ea5a2918"), u256("140388680000000")},
        {Address("0xb2428a150210f5403e1ea9cfee488fdc259132ee"), u256("120675920060000")},
        {Address("0x2fe8768398e489e93c2ca2cdae85a086607866a7"), u256("118828521180000")},
        {Address("0xc0d97ccaca83c69e31c2a81625eebf03ed6721cb"), u256("85564690060000")},
        {Address("0x12008d9141fcdb92ab8805e7f5e39b4e12dfecca"), u256("85478838840000")},
        {Address("0x58d72fb9f1048b3ad4082f06bea68edd503831b5"), u256("75730784780000")},
        {Address("0x27b2a2da79d24df3bcd6d8ef6f141621a366e06c"), u256("74719103840000")},
        {Address("0xe39984086a55dcc55c4edb3d6369d530c4c0081f"), u256("72350419780000")},
        {Address("0x602e65642c52d4fcf4060235f5b51b9936edd2fb"), u256("68967711460000")},
        {Address("0x53ef7c91df1580bb0d96102d15c8764c08ac9297"), u256("68896346300000")},
        {Address("0x6e8a9cb007409cd63d6e3d417966ed6943621c3c"), u256("67777511100000")},
        {Address("0x51efce1fc1cb794e97ba92d8eaa96b993415b3f4"), u256("67228105940000")},
        {Address("0x7d35696ee255ba2cef1f5e496dfadeaa4d5f5c34"), u256("67211276540000")},
        {Address("0x8a7111a1e869934f6981cd5e6dd3d9b6d98e5603"), u256("67168457440000")},
        {Address("0xc84eaee9d79f92d9d1d598968c4283fe0dfa5c5a"), u256("67131390160000")},
        {Address("0xac94ad70351ec72329c1da4a0400923fe17dbde8"), u256("67047456220000")},
        {Address("0xf10762d64bb661691b6ee6ebafd399e5a131563e"), u256("66963948340000")},
        {Address("0x8e7f14bb4cb837fd5947dc99691dca5e802c187d"), u256("66948823200000")},
        {Address("0x9c4aa479eaebae43e8284f03806c9d5131da1067"), u256("66463327120000")},
        {Address("0x6c89959351d710131ee9c789d8f725eb9b6ebef9"), u256("66395157420000")}};
    return _masterNode;
}