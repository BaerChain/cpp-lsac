#include "WebThree.h"

#include <libbrchashseal/Brchash.h>
#include <libbrchashseal/BrchashClient.h>
#include <libbrcdchain/ClientTest.h>
#include <libbrcdchain/BrcdChainCapability.h>

#include <libpoaseal/Poa.h>
#include <libpoaseal/PoaClient.h>

#include <libshdposseal/SHDpos.h>
#include <libshdposseal/SHDposClient.h>

#include <brcd/buildinfo.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::brc;
using namespace dev::shh;

static_assert(BOOST_VERSION >= 106400, "Wrong boost headers version");

WebThreeDirect::WebThreeDirect(std::string const &_clientVersion,
                               boost::filesystem::path const &_dbPath, boost::filesystem::path const &_snapshotPath,
                               brc::ChainParams const &_params, WithExisting _we,
                               std::set<std::string> const &_interfaces,
                               NetworkConfig const &_n, bytesConstRef _network, bool _testing)
        : m_clientVersion(_clientVersion), m_net(_clientVersion, _n, _network) {
    if (_interfaces.count("brc")) {
        if (_testing)
            m_brcdChain.reset(new brc::ClientTest(
                    _params, (int) _params.networkID, m_net, shared_ptr<GasPricer>(), _dbPath, _we));
        else {
            if (_params.sealEngineName == Brchash::name())
                m_brcdChain.reset(new brc::BrchashClient(_params, (int) _params.networkID, m_net,
                                                       shared_ptr<GasPricer>(), _dbPath, _snapshotPath, _we));
            else if (_params.sealEngineName == NoProof::name())
                m_brcdChain.reset(new brc::Client(_params, (int) _params.networkID, m_net,
                                                 shared_ptr<GasPricer>(), _dbPath, _snapshotPath, _we));
            else if (_params.sealEngineName == Poa::name())
                m_brcdChain.reset(new brc::PoaClient(_params, (int) _params.networkID, m_net,
                                                    shared_ptr<GasPricer>(), _dbPath, _snapshotPath, _we));
            else if (_params.sealEngineName == bacd::SHDpos::name())
                m_brcdChain.reset(new bacd::SHDposClient(_params, (int) _params.networkID, m_net,
                                                      shared_ptr<GasPricer>(), _dbPath, _snapshotPath, _we));
            else
                BOOST_THROW_EXCEPTION(ChainParamsInvalid() << errinfo_comment(
                        "Unknown seal engine: " + _params.sealEngineName));
        }
        m_brcdChain->startWorking();
        const auto *buildinfo = brcd_get_buildinfo();
        m_brcdChain->setExtraData(rlpList(0, string{buildinfo->project_version}.substr(0, 5) + "++" +
                                            string{buildinfo->git_commit_hash}.substr(0, 4) +
                                            string{buildinfo->build_type}.substr(0, 1) +
                                            string{buildinfo->system_name}.substr(0, 5) +
                                            string{buildinfo->compiler_id}.substr(0, 3)));
    }
}

WebThreeDirect::~WebThreeDirect() {
    // Utterly horrible right now - WebThree owns everything (good), but:
    // m_net (Host) owns the brc::BrcdChainHost via a shared_ptr.
    // The brc::BrcdChainHost depends on brc::Client (it maintains a reference to the BlockChain field of Client).
    // brc::Client (owned by us via a unique_ptr) uses brc::BrcdChainHost (via a weak_ptr).
    // Really need to work out a clean way of organising ownership and guaranteeing startup/shutdown is perfect.

    // Have to call stop here to get the Host to kill its io_service otherwise we end up with left-over reads,
    // still referencing Sessions getting deleted *after* m_brcdChain is reset, causing bad things to happen, since
    // the guarantee is that m_brcdChain is only reset *after* all sessions have ended (sessions are allowed to
    // use bits of data owned by m_brcdChain).
    m_net.stop();
}

std::string WebThreeDirect::composeClientVersion(std::string const &_client) {
    const auto *buildinfo = brcd_get_buildinfo();
    return _client + "/" + buildinfo->project_version + "/" + buildinfo->system_name + "/" +
           buildinfo->compiler_id + buildinfo->compiler_version + "/" + buildinfo->build_type;
}

std::vector<PeerSessionInfo> WebThreeDirect::peers() {
    return m_net.peerSessionInfo();
}

size_t WebThreeDirect::peerCount() const {
    return m_net.peerCount();
}

void WebThreeDirect::setIdealPeerCount(size_t _n) {
    return m_net.setIdealPeerCount(_n);
}

void WebThreeDirect::setPeerStretch(size_t _n) {
    return m_net.setPeerStretch(_n);
}

bytes WebThreeDirect::saveNetwork() {
    return m_net.saveNetwork();
}

void WebThreeDirect::addNode(p2p::NodeID const &_node, bi::tcp::endpoint const &_host) {
    m_net.addNode(_node, NodeIPEndpoint(_host.address(), _host.port(), _host.port()));
}

void WebThreeDirect::requirePeer(p2p::NodeID const &_node, bi::tcp::endpoint const &_host) {
    m_net.requirePeer(_node, NodeIPEndpoint(_host.address(), _host.port(), _host.port()));
}

void WebThreeDirect::addPeer(NodeSpec const &_s, PeerType _t) {
    m_net.addPeer(_s, _t);
}

void dev::WebThreeDirect::replace_node(bytes &_b, Secret const& _key){
	if(!_key)
		return;
	RLP r(_b);
	if(r.itemCount() == 3 && r[0].isInt() && r[0].toInt<unsigned>() >= 3 && _key != Secret()){
		if(_key == Secret(r[1].toBytes()))
			return;
		RLPStream rlp(3);
		rlp << dev::p2p::c_protocolVersion << _key.ref();
		RLPStream network;
		int count = r[2].itemCount();
        for (auto i : r[2]){
			network.appendRaw(i.data(), i.itemCount());
        }
		if(count){
			rlp.appendList(count);
			rlp.appendRaw(network.out(), count);
		}
		//std::cout << " old data:    " << toString(_b) <<std::endl;
		//std::cout << " old data rlp:" << toString(rlp.out()) << std::endl;
		rlp.swapOut(_b);
	}
}

