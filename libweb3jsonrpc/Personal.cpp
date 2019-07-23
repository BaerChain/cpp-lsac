#include <jsonrpccpp/common/exception.h>
#include <libbrccore/KeyManager.h>
#include <libweb3jsonrpc/AccountHolder.h>
#include <libbrccore/CommonJS.h>
#include <libweb3jsonrpc/JsonHelper.h>
#include <libbrcdchain/Client.h>

#include "Personal.h"

using namespace std;
using namespace dev;
using namespace dev::rpc;
using namespace dev::brc;
using namespace jsonrpc;

Personal::Personal(KeyManager& _keyManager, AccountHolder& _accountHolder, brc::Interface& _brc):
	m_keyManager(_keyManager),
	m_accountHolder(_accountHolder),
	m_brc(_brc)
{
}

std::string Personal::personal_newAccount(std::string const& _password)
{
	KeyPair p = KeyManager::newKeyPair(KeyManager::NewKeyType::NoVanity);
	m_keyManager.import(p.secret(), std::string(), _password, std::string());
	return toJS(p.address());
}

string Personal::personal_sendTransaction(Json::Value const& _transaction, string const& _password)
{
	TransactionSkeleton t;
	try
	{
		t = toTransactionSkeleton(_transaction);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}

	if (Secret s = m_keyManager.secret(t.from, [&](){ return _password; }, false))
	{
		// return the tx hash
        /*enum Type
	    {
		    NullTransaction, ContractCreation, MessageCall, VoteMassage
	    }; 
		u256 type;
		u256 flag;
		bool isVote = false;

        try
		{
		    if(!_transaction["type"].empty() && !_transaction["value"].empty())
			{
			    type = jsToU256(_transaction["type"].asString());
                flag = jsToU256(_transaction["value"].asString());
				if(type == 3)
					isVote = true;
			}
		}
		catch(...)
		{
			BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
		}
        std::cout<<"////////////////////// this transation is vote:"<< (int)isVote << std::endl;
		if(isVote)
			return toJS(m_brc.submitTransaction(t, s, flag));*/
		return toJS(m_brc.submitTransaction(t, s));
        
	}
	BOOST_THROW_EXCEPTION(JsonRpcException("Invalid password or account."));
}

/*string Personal::personal_sendDelegatedTransaction(Json::Value const& _transaction, string const& _password)
{
    std::cout << "///////////////////////personal_sendTransaction " << std::endl;
	TransactionSkeleton t;
    u256 flag;
	try
	{
		t = toTransactionSkeleton(_transaction);
        if (!_transaction["value"].empty())
        flag = jsToU256(_transaction["value"].asString());
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}

	if (Secret s = m_keyManager.secret(t.from, [&](){ return _password; }, false))
	{
		// return the tx hash
		return toJS(m_brc.submitTransaction(t, s, flag));

        //return "yes";
	}
	BOOST_THROW_EXCEPTION(JsonRpcException("Invalid password or account."));
}*/

string Personal::personal_signAndSendTransaction(Json::Value const& _transaction, string const& _password)
{
	return personal_sendTransaction(_transaction, _password);
}

bool Personal::personal_unlockAccount(std::string const& _address, std::string const& _password, int _duration)
{
	return m_accountHolder.unlockAccount(Address(fromHex(_address, WhenError::Throw)), _password, _duration);
}

Json::Value Personal::personal_listAccounts()
{
	return toJson(m_keyManager.accounts());
}
