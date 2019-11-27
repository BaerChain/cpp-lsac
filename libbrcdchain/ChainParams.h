#pragma once
#include <libdevcore/Common.h>
#include <libbrccore/Common.h>
#include <libdevcrypto/Common.h>
#include <libbrccore/ChainOperationParams.h>
#include <libbrccore/BlockHeader.h>
#include <libdevcrypto/base58.h>
#include "Account.h"

namespace dev
{
namespace brc
{

class SealEngineFace;

struct ConnectPeers 
{
	ConnectPeers(Address const& _addr, Public const& _p, std::string const& _s) :
		m_addr(_addr), m_node_id(_p), m_host(_s){}
	Address m_addr;
	Public  m_node_id;
	std::string m_host;
};

struct ChainParams: public ChainOperationParams
{
    ChainParams();
    ChainParams(ChainParams const& /*_org*/) = default;
    ChainParams(std::string const& _s, h256 const& _stateRoot = h256());
    ChainParams(bytes const& _genesisRLP, AccountMap const& _state) { populateFromGenesis(_genesisRLP, _state); }
    ChainParams(std::string const& _json, bytes const& _genesisRLP, AccountMap const& _state): ChainParams(_json) { populateFromGenesis(_genesisRLP, _state); }

    SealEngineFace* createSealEngine();

    /// Genesis params.
    h256 parentHash = h256();
    Address author = Address();
    u256 difficulty = 1;
    u256 gasLimit = 1 << 31;
    u256 gasUsed = 0;
    u256 timestamp = 0;
    bytes extraData;
    mutable h256 stateRoot;    ///< Only pre-populate if known equivalent to genesisState's root. If they're different Bad Things Will Happen.
    AccountMap genesisState;


	size_t   epochInterval = 0;           /// shdpos epoch time if 0 shdpos will only one epoch
	size_t   varlitorInterval = 1000;        /// creater seal block time
	size_t   blockInterval = 1000;           /// a block sealed time

    SignatureStruct     m_sign_data;
    unsigned sealFields = 0;
    bytes sealRLP;

    uint32_t	config_blocks = 12;			//configure block number


    //Poa 
    std::vector<Address> poaValidatorAccount;
	std::vector<Address> poaBlockAccount;

    std::map<Address, Secret>       m_miner_priv_keys;      ///key: address, value : private key , packed block and sign
    std::map<Address, Secret>       m_block_addr_keys;

	std::unordered_map <Public, std::string> m_mpeers;

	std::vector<ConnectPeers> m_peers;
    std::string m_nodemonitorIP;

    h256 calculateStateRoot(bool _force = false) const;

    /// Genesis block info.
    bytes genesisBlock() const;

    /// load config
    ChainParams loadConfig(std::string const& _json, h256 const& _stateRoot = {},
        const boost::filesystem::path& _configPath = {}) const;

    void saveBlockAddress(std::string const& _json);
    void setPrivateKey(std::string const& key_str);

    void savenodemonitorIP(std::string const& _IP) {m_nodemonitorIP = _IP;}
    std::string getnodemonitorIp() const { return m_nodemonitorIP;}

    std::unordered_map <Public, std::string> getConnectPeers() const;
	std::map<Address, Public> getPeersMessage() const;
	u256 getTimestamp() {return timestamp;}

    void setGasPrice(u256 const& _gasPrice){
        m_minGasPrice = _gasPrice;
    }
    u256 getGasPrice() const { return m_minGasPrice;}

private:
    void populateFromGenesis(bytes const& _genesisRLP, AccountMap const& _state);

    /// load genesis
    ChainParams loadGenesis(std::string const& _json, h256 const& _stateRoot = {}) const;
    // laod Poa validators account
    //ChainParams loadpoaValidators(std::string const& _json, h256 const& _stateRoot = {}) const;
};

}
}
