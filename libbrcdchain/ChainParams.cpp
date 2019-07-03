#include "ChainParams.h"
#include "Account.h"
#include "GenesisInfo.h"
#include "State.h"
#include "ValidationSchemes.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/JsonUtils.h>
#include <libdevcore/Log.h>
#include <libdevcore/TrieDB.h>
#include <libbrccore/BlockHeader.h>
#include <libbrccore/Precompiled.h>
#include <libbrccore/SealEngine.h>

using namespace std;
using namespace dev;
using namespace brc;
using namespace brc::validation;
namespace js = json_spirit;

ChainParams::ChainParams()
{
    for (unsigned i = 1; i <= 4; ++i)
        genesisState[Address(i)] = Account(0, 1);
    // Setup default precompiled contracts as equal to genesis of Frontier.
    precompiled.insert(make_pair(Address(1), PrecompiledContract(3000, 0, PrecompiledRegistrar::executor("ecrecover"))));
    precompiled.insert(make_pair(Address(2), PrecompiledContract(60, 12, PrecompiledRegistrar::executor("sha256"))));
    precompiled.insert(make_pair(Address(3), PrecompiledContract(600, 120, PrecompiledRegistrar::executor("ripemd160"))));
    precompiled.insert(make_pair(Address(4), PrecompiledContract(15, 3, PrecompiledRegistrar::executor("identity"))));
}

ChainParams::ChainParams(string const& _json, h256 const& _stateRoot)
{
    *this = loadConfig(_json, _stateRoot);
}

ChainParams ChainParams::loadConfig(
        string const& _json, h256 const& _stateRoot, const boost::filesystem::path& _configPath) const
{
    ChainParams cp(*this);
    js::mValue val;
    js::read_string_or_throw(_json, val);
    js::mObject obj = val.get_obj();

    validateConfigJson(obj);
    cp.sealEngineName = obj[c_sealEngine].get_str();
    // params
    js::mObject params = obj[c_params].get_obj();
    cp.accountStartNonce = u256(fromBigEndian<u256>(fromHex(params[c_accountStartNonce].get_str())));
    cp.maximumExtraDataSize = u256(fromBigEndian<u256>(fromHex(params[c_maximumExtraDataSize].get_str())));
	cp.tieBreakingGas = false; //params.count(c_tieBreakingGas) ? params[c_tieBreakingGas].get_bool() : true;
    if(params.count(c_blockReward))
        cp.setBlockReward(u256(fromBigEndian<u256>(fromHex(params[c_blockReward].get_str()))));

    auto setOptionalU256Parameter = [&params](u256 &_destination, string const& _name)
    {
        if (params.count(_name))
            _destination = u256(fromBigEndian<u256>(fromHex(params.at(_name).get_str())));
    };
    setOptionalU256Parameter(cp.minGasLimit, c_minGasLimit);
    setOptionalU256Parameter(cp.maxGasLimit, c_maxGasLimit);
    setOptionalU256Parameter(cp.gasLimitBoundDivisor, c_gasLimitBoundDivisor);
    setOptionalU256Parameter(cp.homesteadForkBlock, c_homesteadForkBlock);
    setOptionalU256Parameter(cp.EIP150ForkBlock, c_EIP150ForkBlock);
    setOptionalU256Parameter(cp.EIP158ForkBlock, c_EIP158ForkBlock);
    setOptionalU256Parameter(cp.byzantiumForkBlock, c_byzantiumForkBlock);
    setOptionalU256Parameter(cp.eWASMForkBlock, c_eWASMForkBlock);
    setOptionalU256Parameter(cp.constantinopleForkBlock, c_constantinopleForkBlock);
    setOptionalU256Parameter(cp.daoHardforkBlock, c_daoHardforkBlock);
    setOptionalU256Parameter(cp.experimentalForkBlock, c_experimentalForkBlock);
    setOptionalU256Parameter(cp.minimumDifficulty, c_minimumDifficulty);
    setOptionalU256Parameter(cp.difficultyBoundDivisor, c_difficultyBoundDivisor);
    setOptionalU256Parameter(cp.durationLimit, c_durationLimit);

    if (params.count(c_chainID))
        cp.chainID = int(u256(fromBigEndian<u256>(fromHex(params.at(c_chainID).get_str()))));
    if (params.count(c_networkID))
        cp.networkID = int(u256(fromBigEndian<u256>(fromHex(params.at(c_networkID).get_str()))));
    cp.allowFutureBlocks = params.count(c_allowFutureBlocks);

    //dpos data
    //size_t(u256(fromBigEndian<u256>(fromHex(params.at(c_varlitorInterval).get_str()))));
	if(params.count(c_epochInterval))
		cp.epochInterval = size_t(params.at(c_epochInterval).get_int());
	if(params.count(c_varlitorInterval))
		cp.varlitorInterval = size_t(params.at(c_varlitorInterval).get_int());
	if(params.count(c_blockInterval))
		cp.blockInterval = size_t(params.at(c_blockInterval).get_int());
	if(params.count(c_checkVarlitorNum))
		cp.checkVarlitorNum = size_t(params.at(c_checkVarlitorNum).get_int());
	if(params.count(c_maxVarlitorNum))
		cp.maxVarlitorNum = size_t(params.at(c_maxVarlitorNum).get_int());

    ////Poa Validators
    //string poaStr = js::write_string(obj[c_poa], false);
    //cp = cp.loadpoaValidators(poaStr, _stateRoot);
    // genesis
    string genesisStr = js::write_string(obj[c_genesis], false);
    cp = cp.loadGenesis(genesisStr, _stateRoot);
    // genesis state
    string genesisStateStr = js::write_string(obj[c_accounts], false);

    cp.genesisState = jsonToAccountMap(
                genesisStateStr, cp.accountStartNonce, nullptr, &cp.precompiled, _configPath);

    cp.stateRoot = _stateRoot ? _stateRoot : cp.calculateStateRoot(true);

    return cp;
}

ChainParams ChainParams::loadGenesis(string const& _json, h256 const& _stateRoot) const
{
    ChainParams cp(*this);

    js::mValue val;
    js::read_string(_json, val);
    js::mObject genesis = val.get_obj();

    cp.parentHash = h256(0); // required by the YP
    cp.author = genesis.count(c_coinbase) ? h160(genesis[c_coinbase].get_str()) : h160(genesis[c_author].get_str());
    cp.difficulty = genesis.count(c_difficulty) ?
                u256(fromBigEndian<u256>(fromHex(genesis[c_difficulty].get_str()))) :
                cp.minimumDifficulty;
    cp.gasLimit = u256(fromBigEndian<u256>(fromHex(genesis[c_gasLimit].get_str())));
    cp.gasUsed = genesis.count(c_gasUsed) ? u256(fromBigEndian<u256>(fromHex(genesis[c_gasUsed].get_str()))) : 0;
    cp.timestamp = u256(fromBigEndian<u256>(fromHex(genesis[c_timestamp].get_str())));
    cp.extraData = bytes(fromHex(genesis[c_extraData].get_str()));
    cp.m_sign_data = h520(fromHex(genesis[c_sign].get_str()));

	//dposVarlitors.assign(cp.poaValidatorAccount.begin(), cp.poaValidatorAccount.end());

    if (genesis.count(c_mixHash) && genesis.count(c_nonce))
    {
        h256 mixHash(genesis[c_mixHash].get_str());
        h64 nonce(genesis[c_nonce].get_str());
        cp.sealFields = 2;
        cp.sealRLP = rlp(mixHash) + rlp(nonce);
    }
    cp.stateRoot = _stateRoot ? _stateRoot : cp.calculateStateRoot();
    return cp;
}

void dev::brc::ChainParams::saveBlockAddress(std::string const& _json)
{
    try 
	{
		js::mValue val;
		js::read_string_or_throw(_json, val);
		js::mObject _obj = val.get_obj();
		if(_obj.count("private_key"))
		{
			js::mArray private_key_array = _obj["private_key"].get_array();
			for(auto val : private_key_array)
			{
				auto _key = val.get_str();
				auto secret = Secret(dev::crypto::from_base58(_key));
				auto address = toAddress(toPublic(secret));
				this->m_block_addr_keys.insert(pair<Address, Secret>(address, secret));
			}
		}
		if(_obj.count("peer_host"))
		{
			for(auto const& val : _obj["peer_host"].get_array())
			{
				auto item = val.get_obj();
				std::string _id = item["node_id"].get_str();
				std::string _host = item["host"].get_str();
				this->m_peers.push_back({ Address(), Public(_id), _host });
			}
		}
	}
    catch(Exception &ex)
	{
		cerror << "load accountJson error!" << ex.what();
		cerror << "sample: \n" << R"E(
            {
            "private_key":["8RioSGhgNUKFZopC2rR3HRDD78Sc48gci4pkVhsduZve"]
             "peer_host":[
               {
                "node_id":"",
                "host":""
               }
            ]
            }
            )E";
		throw;
	}
   
}

std::unordered_map<Public, std::string> dev::brc::ChainParams::getConnectPeers() const
{
	std::unordered_map<Public, std::string> _map;
	for(auto const& val : m_peers)
		_map.insert({ val.m_node_id, val.m_host });

	return _map;
}

std::map<Address, Public> dev::brc::ChainParams::getPeersMessage() const
{
	std::map<Address, Public> _map;
	for(auto const& val : m_peers)
	{
        if(val.m_addr == Address())
            continue;
		_map.insert({ val.m_addr, val.m_node_id });
	}
	return _map;
}

SealEngineFace* ChainParams::createSealEngine()
{
    SealEngineFace* ret = SealEngineRegistrar::create(sealEngineName);
    assert(ret && "Seal engine not found");
    if (!ret)
        return nullptr;
    ret->setChainParams(*this);
    if (sealRLP.empty())
    {
        sealFields = ret->sealFields();
        sealRLP = ret->sealRLP();
    }
    return ret;
}

void ChainParams::populateFromGenesis(bytes const& _genesisRLP, AccountMap const& _state)
{
    BlockHeader bi(_genesisRLP, RLP(&_genesisRLP)[0].isList() ? BlockData : HeaderData);
    parentHash = bi.parentHash();
    author = bi.author();
    difficulty = bi.difficulty();
    gasLimit = bi.gasLimit();
    gasUsed = bi.gasUsed();
    timestamp = bi.timestamp();
    extraData = bi.extraData();
    m_sign_data = bi.sign_data();
    genesisState = _state;
    RLP r(_genesisRLP);
    sealFields = r[0].itemCount() - BlockHeader::BasicFields;
    sealRLP.clear();
    for (unsigned i = BlockHeader::BasicFields; i < r[0].itemCount(); ++i)
        sealRLP += r[0][i].data();

    calculateStateRoot(true);

    auto b = genesisBlock();
    if (b != _genesisRLP)
    {
        cdebug << "Block passed:" << bi.hash() << bi.hash(WithoutSeal);
        cdebug << "Genesis now:" << BlockHeader::headerHashFromBlock(b);
        cdebug << RLP(b);
        cdebug << RLP(_genesisRLP);
        throw 0;
    }
}

h256 ChainParams::calculateStateRoot(bool _force) const
{
    StateCacheDB db;
    SecureTrieDB<Address, StateCacheDB> state(&db);
    state.init();
    if (!stateRoot || _force)
    {
        // TODO: use hash256
        //stateRoot = hash256(toBytesMap(gs));
        dev::brc::commit(genesisState, state);
        stateRoot = state.root();
    }
    return stateRoot;
}

bytes ChainParams::genesisBlock() const
{
    RLPStream block(3);

    calculateStateRoot();

    block.appendList(BlockHeader::BasicFields + sealFields + 1)
            << parentHash
            << EmptyListSHA3	// sha3(uncles)
            << author
            << stateRoot
            << EmptyTrie		// transactions
            << EmptyTrie		// receipts
            << LogBloom()
            << difficulty
            << 0				// number
            << gasLimit
            << gasUsed			// gasUsed
            << timestamp
            << extraData
            << h520(m_sign_data);

    block.appendRaw(sealRLP, sealFields);
    block.appendRaw(RLPEmptyList);
    block.appendRaw(RLPEmptyList);
    return block.out();
}
