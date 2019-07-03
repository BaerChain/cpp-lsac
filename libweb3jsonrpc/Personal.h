#pragma once
#include "PersonalFace.h"

namespace dev
{
	
namespace brc
{
class KeyManager;
class AccountHolder;
class Interface;
}

namespace rpc
{

class Personal: public dev::rpc::PersonalFace
{
public:
    /*enum Type
	{
		BeCandidate,    // 申请自己成为候选人
		CacleCandidate, // 申请取消自己的候选人资格
		Delegated       // 投票
	};*/
	Personal(dev::brc::KeyManager& _keyManager, dev::brc::AccountHolder& _accountHolder, brc::Interface& _brc);
	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"personal", "1.0"}};
	}
	virtual std::string personal_newAccount(std::string const& _password) override;
	virtual bool personal_unlockAccount(std::string const& _address, std::string const& _password, int _duration) override;
	virtual std::string personal_signAndSendTransaction(Json::Value const& _transaction, std::string const& _password) override;
	virtual std::string personal_sendTransaction(Json::Value const& _transaction, std::string const& _password) override;
    //virtual std::string personal_sendDelegatedTransaction(Json::Value const& _transaction, std::string const& _password) override;
	virtual Json::Value personal_listAccounts() override;

private:
	dev::brc::KeyManager& m_keyManager;
	dev::brc::AccountHolder& m_accountHolder;
	dev::brc::Interface& m_brc;
};

}
}
