#include "Account.h"
#include "SecureTrieDB.h"
#include "ValidationSchemes.h"
#include <libdevcore/JsonUtils.h>
#include <libdevcore/OverlayDB.h>
#include <libbrccore/ChainOperationParams.h>
#include <libbrccore/Precompiled.h>
#include <libdevcore/Log.h>

using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace dev::brc::validation;

namespace fs = boost::filesystem;

void Account::setCode(bytes&& _code)
{
    m_codeCache = std::move(_code);
    m_hasNewCode = true;
    m_codeHash = sha3(m_codeCache);
}

u256 Account::originalStorageValue(u256 const& _key, OverlayDB const& _db) const
{
    auto it = m_storageOriginal.find(_key);
    if (it != m_storageOriginal.end())
        return it->second;

    // Not in the original values cache - go to the DB.
    SecureTrieDB<h256, OverlayDB> const memdb(const_cast<OverlayDB*>(&_db), m_storageRoot);
    std::string const payload = memdb.at(_key);
    auto const value = payload.size() ? RLP(payload).toInt<u256>() : 0;
    m_storageOriginal[_key] = value;
    return value;
}

void dev::brc::Account::addVote(std::pair<Address, u256> _votePair)
{
    auto ret = m_voteData.find(_votePair.first);
    if(ret == m_voteData.end())
    {
        if(_votePair.second)
        {
            m_voteData.insert(_votePair);
            changed();
            return;
        }
    }
    if(ret->second + _votePair.second > 0)
        ret->second += _votePair.second;
    else
        m_voteData.erase(ret);
	changed();
}

void dev::brc::Account::addBlockRewardRecoding(std::pair<u256, u256> _pair)
{
    if(m_BlockReward.size() == 0)
    {
        m_BlockReward.push_back(_pair);
    }else{
        auto _rewardPair = m_BlockReward.back();
        if(_rewardPair.first == _pair.first)
        {
            _rewardPair.second += _pair.second;
            m_BlockReward.pop_back();
            m_BlockReward.push_back(_rewardPair);
        }else{
            m_BlockReward.push_back(_pair);
        }
    }
    changed();
}

void dev::brc::Account::manageSysVote(Address const& _otherAddr, bool _isLogin, u256 _tickets)
{
	// 该接口 保留票数为0的数据  当是成为或者撤销竞选人是否，_tickets 为0
	auto ret = m_voteData.find(_otherAddr);
	if(_isLogin && ret == m_voteData.end())
	{
		m_voteData[_otherAddr] = _tickets;
	}
	else if(!_isLogin && ret != m_voteData.end())
		m_voteData.erase(ret);
	changed();
}

bool dev::brc::Account::insertMiner(Address before, Address after, unsigned blockNumber)
{
    std::string tmp = before.hex() + ":" + after.hex() + ":" + to_string(blockNumber);
    m_willChangeList.push_back(tmp);
    changed();
    return true;
}

bool dev::brc::Account::changeVoteData(Address before, Address after)
{
    std::map<Address, u256>::iterator del = m_voteData.find(before);
    if (del != m_voteData.end()){
        u256 tmp = m_voteData[before];
        m_voteData.erase(del);
        m_voteData[after] = tmp;
        changed();
        return true;
    }
    return false;
}

bool dev::brc::Account::changeMiner(unsigned blockNumber)
{
    if (m_willChangeList.size() > 0){
        for(auto it = m_willChangeList.begin(); it != m_willChangeList.end();){
            char before[128] = "";
            char after[128] = "";
            unsigned number = 0;
            sscanf((*it).c_str(), "%[^:]:%[^:]:%u", before, after, &number);
            cwarn << "change miner from " << before << " to " << after;
            Address _before(before);
            Address _after(after);
            if(blockNumber >= number){
                changeVoteData(_before, _after);
                it = m_willChangeList.erase(it);
            }else{
                it++;
            }
        }
        changed();
    }
    
    return true;
}

namespace js = json_spirit;

namespace
{

uint64_t toUnsigned(js::mValue const& _v)
{
    switch (_v.type())
    {
    case js::int_type: return _v.get_uint64();
    case js::str_type: return fromBigEndian<uint64_t>(fromHex(_v.get_str()));
    default: return 0;
    }
}

PrecompiledContract createPrecompiledContract(js::mObject const& _precompiled)
{
    auto n = _precompiled.at("name").get_str();
    try
    {
        u256 startingBlock = 0;
        if (_precompiled.count("startingBlock"))
            startingBlock = u256(_precompiled.at("startingBlock").get_str());

        if (!_precompiled.count("linear"))
            return PrecompiledContract(PrecompiledRegistrar::pricer(n), PrecompiledRegistrar::executor(n), startingBlock);

        auto const& l = _precompiled.at("linear").get_obj();
        unsigned base = toUnsigned(l.at("base"));
        unsigned word = toUnsigned(l.at("word"));
        return PrecompiledContract(base, word, PrecompiledRegistrar::executor(n), startingBlock);
    }
    catch (PricerNotFound const&)
    {
        cwarn << "Couldn't create a precompiled contract account. Missing a pricer called:" << n;
        throw;
    }
    catch (ExecutorNotFound const&)
    {
        // Oh dear - missing a plugin?
        cwarn << "Couldn't create a precompiled contract account. Missing an executor called:" << n;
        throw;
    }
}
}

AccountMap dev::brc::jsonToAccountMap(std::string const& _json, u256 const& _defaultNonce,
    AccountMaskMap* o_mask, PrecompiledContractMap* o_precompiled, const fs::path& _configPath)
{
    auto u256Safe = [](std::string const& s) -> u256 {
        bigint ret(s);
        if (ret >= bigint(1) << 256)
            BOOST_THROW_EXCEPTION(
                ValueTooLarge() << errinfo_comment("State value is equal or greater than 2**256"));
        return (u256)ret;
    };

    std::unordered_map<Address, Account> ret;

    js::mValue val;
    json_spirit::read_string_or_throw(_json, val);

    for (auto const& account : val.get_obj())
    {
        Address a(fromHex(account.first));
        auto const& accountMaskJson = account.second.get_obj();

        bool haveBalance = (accountMaskJson.count(c_wei) || accountMaskJson.count(c_finney) ||
                            accountMaskJson.count(c_balance));
        bool haveNonce = accountMaskJson.count(c_nonce);
        bool haveCode = accountMaskJson.count(c_code) || accountMaskJson.count(c_codeFromFile);
        bool haveStorage = accountMaskJson.count(c_storage);
        bool shouldNotExists = accountMaskJson.count(c_shouldnotexist);
		bool haveGenesisCreator = accountMaskJson.count(c_genesisVarlitor);
		bool haveCurrency = accountMaskJson.count(c_currency);

        if (haveStorage || haveCode || haveNonce || haveBalance)
        {
            u256 balance = 0;
            if (accountMaskJson.count(c_wei))
                balance = u256Safe(accountMaskJson.at(c_wei).get_str());
            else if (accountMaskJson.count(c_finney))
                balance = u256Safe(accountMaskJson.at(c_finney).get_str()) * finney;
            else if (accountMaskJson.count(c_balance))
                balance = u256Safe(accountMaskJson.at(c_balance).get_str());

            u256 nonce =
                haveNonce ? u256Safe(accountMaskJson.at(c_nonce).get_str()) : _defaultNonce;

            ret[a] = Account(nonce, balance);
            auto codeIt = accountMaskJson.find(c_code);
            if (codeIt != accountMaskJson.end())
            {
                auto& codeObj = codeIt->second;
                if (codeObj.type() == json_spirit::str_type)
                {
                    auto& codeStr = codeObj.get_str();
                    if (codeStr.find("0x") != 0 && !codeStr.empty())
                        cerr << "Error importing code of account " << a
                             << "! Code needs to be hex bytecode prefixed by \"0x\".";
                    else
                        ret[a].setCode(fromHex(codeStr));
                }
                else
                    cerr << "Error importing code of account " << a
                         << "! Code field needs to be a string";
            }

            auto codePathIt = accountMaskJson.find(c_codeFromFile);
            if (codePathIt != accountMaskJson.end())
            {
                auto& codePathObj = codePathIt->second;
                if (codePathObj.type() == json_spirit::str_type)
                {
                    fs::path codePath{codePathObj.get_str()};
                    if (codePath.is_relative())  // Append config dir if code file path is relative.
                        codePath = _configPath.parent_path() / codePath;
                    bytes code = contents(codePath);
                    if (code.empty())
                        cerr << "Error importing code of account " << a << "! Code file "
                             << codePath << " empty or does not exist.\n";
                    ret[a].setCode(std::move(code));
                }
                else
                    cerr << "Error importing code of account " << a
                         << "! Code file path must be a string\n";
            }

            if (haveStorage)
                for (pair<string, js::mValue> const& j : accountMaskJson.at(c_storage).get_obj())
                    ret[a].setStorage(u256(j.first), u256(j.second.get_str()));
        }

        if (o_mask)
        {
            (*o_mask)[a] =
                AccountMask(haveBalance, haveNonce, haveCode, haveStorage, shouldNotExists);
            if (!haveStorage && !haveCode && !haveNonce && !haveBalance &&
                shouldNotExists && !haveGenesisCreator)  // defined only shouldNotExists field
                ret[a] = Account(0, 0);
        }

        if (o_precompiled && accountMaskJson.count(c_precompiled))
        {
            js::mObject p = accountMaskJson.at(c_precompiled).get_obj();
            o_precompiled->insert(make_pair(a, createPrecompiledContract(p)));
        }

        if ( haveGenesisCreator)
        {
			ret[a] = Account(0, 0);
			js::mArray creater = accountMaskJson.at(c_genesisVarlitor).get_array();
			for(auto const& val : creater)
			{
			    Address _addr= Address(val.get_str());
				ret[a].manageSysVote(_addr, true, 0);
			}
        }

		if (haveCurrency)
		{
			u256 cookie = 0;
			u256 fcookie = 0;
			u256 BRC = 0;
			js::mObject _v = accountMaskJson.at(c_currency).get_obj();
			if (_v.count(c_brc))
			{
				BRC = u256Safe(_v["brc"].get_str());
			}
			if (_v.count(c_balance))
			{
				cookie = u256Safe(_v["cookies"].get_str());
			}
			if (_v.count(c_fcookie))
			{
				fcookie = u256Safe(_v["fcookie"].get_str());
			}
			ret[a] = Account(0, cookie, BRC, fcookie);
		}
    }

    return ret;
}
