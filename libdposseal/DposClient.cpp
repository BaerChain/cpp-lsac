#include "DposClient.h"
#include "Dpos.h"
#include "DposHostCapability.h"
#include <boost/filesystem/path.hpp>
#include <libdevcore/Log.h>
#include <time.h>
using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;
using namespace dev::bacd;
namespace fs = boost::filesystem;

DposClient& dev::bacd::asDposClient(Interface& _c)
{
    if (dynamic_cast<Dpos*>(_c.sealEngine()))
        return dynamic_cast<DposClient&>(_c);
    throw InvalidDposSealEngine();
}

DposClient* dev::bacd::asDposClient(Interface* _c)
{
    if (dynamic_cast<Dpos*>(_c->sealEngine()))
        return &dynamic_cast<DposClient&>(*_c);
    throw InvalidDposSealEngine();
}

dev::bacd::DposClient::DposClient(ChainParams const& _params, int _networkID, p2p::Host& _host,
    std::shared_ptr<GasPricer> _gpForAdoption, boost::filesystem::path const& _dbPath,
    boost::filesystem::path const& _snapshotPath, WithExisting _forceAction,
    TransactionQueue::Limits const& _l)
  : Client(_params, _networkID, _host, _gpForAdoption, _dbPath, _snapshotPath, _forceAction, _l)
{
    // will throw if we're not an dpos seal engine.
    asDposClient(*this);
    m_timespace = 5000; 
    m_params = _params;
}

dev::bacd::DposClient::~DposClient() 
{
    // to wake up the thread from Client::doWork()
    m_signalled.notify_all();
    terminate();
}

Dpos* dev::bacd::DposClient::dpos() const
{
    return dynamic_cast<Dpos*>(Client::sealEngine());
}

void dev::bacd::DposClient::startSealing()
{
    setName("DposClient");
    //TODO DposCLient func
    cdebug << "DposClient: into startSealing";
    Client::startSealing();
}

void dev::bacd::DposClient::rejigSealing() 
{
    if (!m_wouldSeal)
        return;
    //打包出块验证 包括出块周期，出块时间，出块人验证
	uint64_t _time = time(NULL);            //这里得到的是系统时间，考虑使用网络心跳同步时间最佳
	if(!isBlockSeal(_time))
		return;
    Client::rejigSealing();
}

void dev::bacd::DposClient::init(p2p::Host & _host, int _netWorkId)
{
	//关联 host 管理的CapabilityHostFace 接口
	cdebug << "capabilityHost :: DposHostCapability";
	auto ethCapability = make_shared<DposHostcapality>(_host.capabilityHost(),
		_netWorkId,
		[this](NodeID _nodeid, unsigned _id, RLP const& _r){
		dpos()->onPoaMsg(_nodeid, _id, _r);
	},
		[this](NodeID const& _nodeid, u256 const& _peerCapabilityVersion){
		dpos()->requestStatus(_nodeid, _peerCapabilityVersion);
	}
	);
	_host.registerCapability(ethCapability);
	dpos()->initEnv(ethCapability);
	dpos()->startGeneration();
}

bool dev::bacd::DposClient::isBlockSeal(uint64_t _now)
{
	//验证时间 考虑创世区块时间
	if(m_bc.info().timestamp() > 0)
	{
		if(!dpos()->checkDeadline(m_bc.info().timestamp(), _time))
			return;
		if(!dpos()->isBolckSeal(m_bc.info().timestamp(), _time))
			return;
	}
	else
	{
		//考虑如果为0 创世区块
		if(m_bc.number() == 0)
			if(!dpos()->isBolckSeal(0, _time))
				return;
	}
	return false;
}

bool dev::bacd::DposClient::applyDposMassage(DposPacketType _type, Address _targe_address)
{
	cdebug << "applyDposMassage:|" << _type <<"|"<< _targe_address;
	return false;
}
