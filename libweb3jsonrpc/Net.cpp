#include <libwebthree/WebThree.h>
#include <libdevcore/CommonJS.h>
#include <libbrccore/Common.h>
#include "Net.h"

using namespace dev;
using namespace dev::rpc;

Net::Net(NetworkFace& _network): m_network(_network) {}

std::string Net::net_version()
{
	return toString(m_network.networkId());
}

std::string Net::net_peerCount()
{
	return toJS(m_network.peerCount());
}

bool Net::net_listening()
{
	return m_network.isNetworkStarted();
}
