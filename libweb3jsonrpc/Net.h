#pragma once
#include "NetFace.h"

namespace dev
{

class NetworkFace;

namespace rpc
{

class Net: public NetFace
{
public:
	Net(NetworkFace& _network);
	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"net", "1.0"}};
	}
	virtual std::string net_version() override;
	virtual std::string net_peerCount() override;
	virtual bool net_listening() override;

private:
	NetworkFace& m_network;
};

}
}
