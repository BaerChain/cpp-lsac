#pragma once
#include <libbrccore/KeyManager.h>

/**
 * Add account management functionnalities to CLI.
 * account list
 * account new
 * account update
 * account import
 * wallet import (import presale wallet)
 */
class AccountManager
{
public:
	/// uses @a argc, @a argv provided by the CLI and executes implemented options.
	bool execute(int argc, char** argv);
	/// stream account help section
	void static streamAccountHelp(std::ostream& _out);
	/// stream wallet help section
	void static streamWalletHelp(std::ostream& _out);

private:
	/// ask end user to create a password.
	std::string createPassword(std::string const& _prompt) const;
	/// creates a ramdom secret/address pair. It uses ICAP.
	dev::KeyPair makeKey() const;
	/// instanciate KeyManager and open the wallet.
	bool openWallet();

	std::unique_ptr<dev::brc::KeyManager> m_keyManager;
};


