/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file ChainParams.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once
#include <libdevcore/Common.h>
#include <libethcore/Common.h>
#include <libdevcrypto/Common.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/BlockHeader.h>
#include <libdevcrypto/base58.h>
#include "Account.h"

namespace dev
{
namespace eth
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
