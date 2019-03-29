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


	size_t   epochInterval = 1;           // һ��������ѯ���� ms
	size_t   varlitorInterval = 1;        // һ��������һ�γ���ʱ��
	size_t   blockInterval = 1;           // һ������̳���ʱ�� ms
	size_t   checkVarlitorNum = 1;        // ɸѡ��֤�˵��������ֵ
	size_t   maxVarlitorNum = 1;          // �����֤������
	size_t   verifyVoteNum = 1;           // ͶƱ����ȷ����

    SignatureStruct     m_sign_data;
    unsigned sealFields = 0;
    bytes sealRLP;

    //Poa 
    std::vector<Address> poaValidatorAccount;

    std::map<Address, Secret>       m_miner_priv_keys;      ///key: address, value : private key , packed block and sign

    h256 calculateStateRoot(bool _force = false) const;

    /// Genesis block info.
    bytes genesisBlock() const;

    /// load config
    ChainParams loadConfig(std::string const& _json, h256 const& _stateRoot = {},
        const boost::filesystem::path& _configPath = {}) const;

private:
    void populateFromGenesis(bytes const& _genesisRLP, AccountMap const& _state);

    /// load genesis
    ChainParams loadGenesis(std::string const& _json, h256 const& _stateRoot = {}) const;
    // laod Poa validators account
    ChainParams loadpoaValidators(std::string const& _json, h256 const& _stateRoot = {}) const;
};

}
}
