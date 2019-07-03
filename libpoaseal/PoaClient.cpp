#include "PoaClient.h"
#include "Common.h"
#include <time.h>

using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace p2p;
namespace fs = boost::filesystem;

PoaClient& dev::brc::asPoaClient(Interface& _c)
{
    if (dynamic_cast<Poa*>(_c.sealEngine()))
        return dynamic_cast<PoaClient&>(_c);
    throw InvalidPoaSealEngine();
}

PoaClient* dev::brc::asPoaClient(Interface* _c)
{
    if (dynamic_cast<Poa*>(_c->sealEngine()))
        return &dynamic_cast<PoaClient&>(*_c);
    throw InvalidPoaSealEngine();
}

dev::brc::PoaClient::PoaClient(ChainParams const& _params, int _networkID, p2p::Host& _host,
    std::shared_ptr<GasPricer> _gpForAdoption, boost::filesystem::path const& _dbPath,
    boost::filesystem::path const& _snapshotPath, WithExisting _forceAction,
    TransactionQueue::Limits const& _l)
  : Client(_params, _networkID, _host, _gpForAdoption, _dbPath, _snapshotPath, _forceAction, _l)
{
    // will throw if we're not an dpos seal engine.
    asPoaClient(*this);
    m_timespace = 5000;
    m_params = _params;
	init(_host, _networkID);
}

dev::brc::PoaClient::~PoaClient()
{
    // to wake up the thread from Client::doWork()
    m_signalled.notify_all();
    terminate();
}

void dev::brc::PoaClient::init(p2p::Host& _host, int _networkID)
{
    //关联 host 管理的CapabilityHostFace 接口
	cdebug << "capabilityHost :: PoaHostCapability";
	auto brcCapability = make_shared<PoaHostCapability>(_host.capabilityHost(), 
		_networkID,
		[this](NodeID _nodeid, unsigned _id, RLP const& _r){
		poa()->onPoaMsg(_nodeid, _id, _r);
	    },
		[this](NodeID const& _nodeid, u256 const& _peerCapabilityVersion){
			poa()->requestStatus(_nodeid, _peerCapabilityVersion);
		}
		);
	_host.registerCapability(brcCapability);
	poa()->initPoaValidatorAccounts(m_params.poaValidatorAccount);
	poa()->initEnv(brcCapability);
	poa()->startGeneration();
}

Poa* dev::brc::PoaClient::poa() const
{
    return dynamic_cast<Poa*>(Client::sealEngine());
}

void dev::brc::PoaClient::doWork(bool _doWait)
{
    //重载dowork 写入poa逻辑
    bool t = true;
    if (m_syncBlockQueue.compare_exchange_strong(t, false))
        syncBlockQueue();

    if (m_needStateReset)
    {
        resetState();
        m_needStateReset = false;
    }

    t = true;
    bool isSealed = false;
    DEV_READ_GUARDED(x_working)
    isSealed = m_working.isSealed();

    if (!isSealed && !isMajorSyncing() && !m_remoteWorking &&
        m_syncTransactionQueue.compare_exchange_strong(t, false))
        syncTransactionQueue();

    tick();

    rejigSealing();

    callQueuedFunctions();

    DEV_READ_GUARDED(x_working)
    isSealed = m_working.isSealed();
    // If the block is sealed, we have to wait for it to tickle through the block queue
    // (which only signals as wanting to be synced if it is ready).
    if (!m_syncBlockQueue && !m_syncTransactionQueue && (_doWait || isSealed) && isWorking())
    {
        std::unique_lock<std::mutex> l(x_signalled);
        m_signalled.wait_for(l, chrono::seconds(2));
    }
}

void dev::brc::PoaClient::startSealing()
{
    setName("PoaClient");
    // TODO PoaCLient func
    // cdebug << "PoaClient: into startSealing";
    Client::startSealing();
}

void dev::brc::PoaClient::stopSealing()
{
    Client::stopSealing();
    // TODO dposCleint func
    cdebug << "PoaClient: into stopSealing";
}

void dev::brc::PoaClient::rejigSealing()
{
    if (!m_wouldSeal)
        return;
    // TODO dopsClient func
    //先控制定时出块
    if (chrono::system_clock::now() - m_lastBlock < chrono::milliseconds(m_timespace))
    {
        return;
    }
    //验证 验证人
	if(!poa()->isvalidators(author(), m_bc.info().author()))
        return;
    m_lastBlock = chrono::system_clock::now();
    // cdebug << "PoaClient: into rejigSealing";
    Client::rejigSealing();
}

void dev::brc::PoaClient::syncBlockQueue()
{
    // cdebug << "PoaClient: into syncBlockQueue";
    Client::syncBlockQueue();
    // TODO dposClient func
}

bool dev::brc::PoaClient::setValidator(const std::string& _address, bool _ret)
{
    //更改权限
    //cdebug << "_address:" << _address << "|| _ret:" << _ret ;
    //判断是否有权限更改验证人  在创世配置的验证人才有权限，，且 创世配置的验证人可以被撤销
    bool ret = false;
    for (auto val : m_params.poaValidatorAccount)
    {
        //cdebug<<"val"<<val<<"\n author():"<< author();
		if (val == author())
		{
            ret = true;
            break;
		}
    }
    if (!ret)
    {
        cwarn << "currAddress:" <<author()<< " can't to Set not have Permission";
        return false;
    }
    // 更改验证人
	return	poa()->updateValitor(Address(_address), _ret, time(NULL));
}

/*
    验证人更改
    ret:true 添加
*/
bool dev::brc::PoaClient::setValidatorAccount(const std::string& _address, bool _ret)
{
    bool ret = setValidator(_address, _ret);
    // TODO 通知其他节点
    return ret;
}