#pragma once
#include "AdminBrcFace.h"

namespace dev
{

namespace brc
{
class Client;
class TrivialGasPricer;
class KeyManager;
}

namespace rpc
{

class SessionManager;

class AdminBrc: public AdminBrcFace
{
public:
	AdminBrc(brc::Client& _brc, brc::TrivialGasPricer& _gp, brc::KeyManager& _keyManager, SessionManager& _sm);

	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"admin", "1.0"}, RPCModule{"miner", "1.0"}};
	}

	virtual bool admin_brc_setMining(bool _on, std::string const& _session) override;
	virtual Json::Value admin_brc_blockQueueStatus(std::string const& _session) override;
	virtual bool admin_brc_setAskPrice(std::string const& _wei, std::string const& _session) override;
	virtual bool admin_brc_setBidPrice(std::string const& _wei, std::string const& _session) override;
	virtual Json::Value admin_brc_findBlock(std::string const& _blockHash, std::string const& _session) override;
	virtual std::string admin_brc_blockQueueFirstUnknown(std::string const& _session) override;
	virtual bool admin_brc_blockQueueRetryUnknown(std::string const& _session) override;
	virtual Json::Value admin_brc_allAccounts(std::string const& _session) override;
	virtual Json::Value admin_brc_newAccount(const Json::Value& _info, std::string const& _session) override;
	virtual bool admin_brc_setMiningBenefactor(std::string const& _uuidOrAddress, std::string const& _session) override;
	virtual Json::Value admin_brc_inspect(std::string const& _address, std::string const& _session) override;
	virtual Json::Value admin_brc_reprocess(std::string const& _blockNumberOrHash, std::string const& _session) override;
	virtual Json::Value admin_brc_vmTrace(std::string const& _blockNumberOrHash, int _txIndex, std::string const& _session) override;
	virtual Json::Value admin_brc_getReceiptByHashAndIndex(std::string const& _blockNumberOrHash, int _txIndex, std::string const& _session) override;
	virtual bool miner_start(int _threads) override;
	virtual bool miner_stop() override;
	virtual bool miner_setBrcerbase(std::string const& _uuidOrAddress) override;
	virtual bool miner_setExtra(std::string const& _extraData) override;
	virtual bool miner_setGasPrice(std::string const& _gasPrice) override;
	virtual std::string miner_hashrate() override;

	virtual void setMiningBenefactorChanger(std::function<void(Address const&)> const& _f) { m_setMiningBenefactor = _f; }
private:
	brc::Client& m_brc;
	brc::TrivialGasPricer& m_gp;
	brc::KeyManager& m_keyManager;
	SessionManager& m_sm;
	std::function<void(Address const&)> m_setMiningBenefactor;

	h256 blockHash(std::string const& _blockNumberOrHash) const;
};

}
}
