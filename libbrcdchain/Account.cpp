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
    auto  ret = std::find(m_vote_data.begin(), m_vote_data.end(), _votePair.first);
    if(ret == m_vote_data.end() && _votePair.second)
    {
        m_vote_data.push_back({_votePair.first, _votePair.second, 0});
        changed();
        return;
    }
    if(ret->m_poll + _votePair.second > 0)
        ret->m_poll += _votePair.second;
    else
        m_vote_data.erase(ret);
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

void Account::manageSysVote(Address const &_otherAddr, bool _isLogin, u256 _tickets, int64_t _time) {
    // The interface retains data of 0 votes when becoming or revoking whether, _tickets is 0
    auto ret = std::find(m_vote_data.begin(), m_vote_data.end(), _otherAddr);
    if (_isLogin && ret == m_vote_data.end()){
        m_vote_data.emplace_back(_otherAddr, _tickets, _time);
    }
    else if (!_isLogin && ret != m_vote_data.end())
        m_vote_data.erase(ret);
    changed();
}

void Account::set_system_poll(const PollData& _p) {
    auto ret = std::find(m_vote_data.begin(), m_vote_data.end(), _p.m_addr);
    if (ret == m_vote_data.end()){
        m_vote_data.emplace_back(_p.m_addr, _p.m_poll, _p.m_time);
    }
    ret->m_poll = _p.m_poll;
    ret->m_time = _p.m_time;
    changed();
}


PollData Account::poll_data(Address const &_addr) const {
    auto ret = std::find(m_vote_data.begin(), m_vote_data.end(), _addr);
    if (ret != m_vote_data.end()){
        return  *ret;
    }
    return PollData();
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
    auto  ret_del = std::find(m_vote_data.begin(), m_vote_data.end(), before);
    if (ret_del != m_vote_data.end()){
        ret_del->m_addr = after;
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
            Address _before(before);
            Address _after(after);
            if(blockNumber >= number){
                changeVoteData(_before, _after);
                cwarn << "blockNumber is " << blockNumber << ",change miner from " << before << " to " << after;
                it = m_willChangeList.erase(it);
            }else{
                it++;
            }
        }
        changed();
    }
    
    return true;
}

std::unordered_map<Address, u256> dev::brc::Account::findSnapshotSummary(uint32_t _snapshotNum)
{
//    if(m_cookieSummary.count(_snapshotNum))
//    {
//        std::unordered_map<uint32_t, std::unordered_map<Address, u256>>::const_iterator _it = m_cookieSummary.find(_snapshotNum);
//        return _it->second;
//    }else{
//        return  std::unordered_map<Address, u256>();
//    }
    return std::unordered_map<Address, u256>();

}

u256 dev::brc::Account::findSnapshotSummaryForAddr(uint32_t _snapshotNum, dev::Address _addr)
{
    std::unordered_map<Address, u256> _map = findSnapshotSummary(_snapshotNum);
    if(_map.count(_addr))
    {
        std::unordered_map<Address, u256>::const_iterator _addrIt = _map.find(_addr);
        return _addrIt->second;
    }
    else{
        return u256(0);
    }
}

void Account::try_new_snapshot(u256 _rounds) {
    u256 summary_cooike =CookieIncome();
    for (u256 j = m_vote_sapshot.m_latest_round+1; j <= _rounds ; ++j) {
        if (!m_vote_sapshot.m_blockSummaryHistory.count(j)){
            std::map<Address, u256> _temp ;
            for(auto const& val: m_vote_data){
                _temp[val.m_addr] = val.m_poll;
            }
            m_vote_sapshot.m_voteDataHistory[j] = _temp;
        }
        if (!m_vote_sapshot.m_pollNumHistory.count(j)){
            m_vote_sapshot.m_pollNumHistory[j] = poll();
        }
        if (!m_vote_sapshot.m_blockSummaryHistory.count(j)){
            m_vote_sapshot.m_blockSummaryHistory[j] = summary_cooike;
            summary_cooike = 0;
        }
    }
    m_vote_sapshot.m_latest_round = _rounds;
}

VoteSnapshot Account::try_new_temp_snapshot(u256 _rounds){
    VoteSnapshot vote_snap = m_vote_sapshot;
    try_new_snapshot(_rounds);
    VoteSnapshot vote1 = m_vote_sapshot;
    m_vote_sapshot = vote_snap;
    return vote1;
}

std::pair<bool, u256> Account::get_no_record_snapshot(u256 _rounds, Votingstage _state) {
    if (_rounds == 0)
        return  std::make_pair(false, 0);
    u256 last_round = --_rounds;
    if (last_round <= 0 || last_round <= m_vote_sapshot.m_latest_round)
        return std::make_pair(false, 0);
    return  std::make_pair(true, last_round);
}



void Account::tryRecordSnapshot(u256 _rounds,  u256 brc, u256 balance, std::vector<PollData>const& p_datas)
{
    if (_rounds <= m_couplingSystemFee.m_rounds)
        return;
//    if(!m_couplingSystemFee.m_Feesnapshot.count(_rounds - 1))
//    {
//        m_couplingSystemFee.m_Feesnapshot[_rounds - 1] = std::pair<u256, u256> (u256(0), u256(0));
//    }
    m_couplingSystemFee.m_Feesnapshot[_rounds-1] = std::pair<u256, u256> (brc, balance);
    m_couplingSystemFee.m_rounds = _rounds - 1;

    std::vector<PollData> snapshot_data;
    uint32_t  index = config::minner_rank_num() +1;
    for(auto const& val: p_datas){
        if (--index)
            snapshot_data.emplace_back(val);
    }
    m_couplingSystemFee.m_sorted_creaters[_rounds - 1] = snapshot_data;
    changed();
}

bool Account::addExchangeOrder(dev::brc::ex::order const &_order)
{
    if(_order.type == dev::brc::ex::order_type::buy)
    {
        if(m_buyExchange.count(_order.trxid))
        {
            return  false;
        }else{
            m_buyExchange.insert(std::make_pair(_order.trxid, _order));
        }
    }else if(_order.type == dev::brc::ex::order_type::sell)
    {
        if(m_sellExchange.count(_order.trxid))
        {
            return false;
        }
    }
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
        bool haveVote = accountMaskJson.count(c_vote);

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
			if (!ret.count(SysElectorAddress))
                ret[SysElectorAddress] = Account(0,0);
			js::mArray creater = accountMaskJson.at(c_genesisVarlitor).get_array();
			for(auto const& val : creater)
			{
			    Address _addr= Address(val.get_str());
                ret[a].manageSysVote(_addr, true, 0);
                ret[SysElectorAddress].manageSysVote(_addr, true, 0);
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
            auto it = ret.find(a);
            if (it == ret.end()) {
                ret[a] = Account(0, cookie, BRC, fcookie);
            } else {
                ret[a].addBalance(cookie);
                ret[a].addBRC(BRC);
                ret[a].addFBRC(fcookie);
            }
		}
        if (haveVote)
		{
			js::mObject _v = accountMaskJson.at(c_vote).get_obj();
			for (auto voteData : _v){
                Address to(voteData.first);
                u256 ballots(voteData.second.get_str());
                auto it = ret.find(to);
                
                if (it != ret.end()){
                    //it->second.addBallot(ballots);
                    it->second.addPoll(ballots);
                } else {
                    ret[to] = Account(0);
                    ret[to].addPoll(ballots);
                }
                it = ret.find(a);
                if (it == ret.end()) {
                    ret[a] = Account(0);
                }
                ret[a].addVote(std::make_pair(to, ballots));
                u256 _ballot = 0;

                if (ret.count(SysVarlitorAddress) && ret[SysVarlitorAddress].poll_data(to) == to){
                    _ballot = ret[SysVarlitorAddress].poll_data(to).m_poll;
                    ret[SysVarlitorAddress].set_system_poll({to, ballots+ _ballot, 0});
                    ret[SysVarlitorAddress].sort_vote_data();
                }
                else if (ret.count(SysCanlitorAddress) && ret[SysCanlitorAddress].poll_data(to) == to){
                    _ballot = ret[SysCanlitorAddress].poll_data(to).m_poll;
                    ret[SysCanlitorAddress].set_system_poll({to, ballots+ _ballot, 0});
                    ret[SysCanlitorAddress].sort_vote_data();
                }

			}
		}
    }

    return ret;
}

