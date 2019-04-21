#include "SHDposClient.h"
#include "SHDpos.h"
#include "SHDposHostCapability.h"
#include <boost/filesystem/path.hpp>
#include <libdevcore/Log.h>
#include <time.h>
using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace p2p;
using namespace dev::bacd;
namespace fs = boost::filesystem;

SHDposClient& dev::bacd::asDposClient(Interface& _c)
{
    if (dynamic_cast<SHDpos*>(_c.sealEngine()))
        return dynamic_cast<SHDposClient&>(_c);
    throw InvalidSHDposSealEngine();
}

SHDposClient* dev::bacd::asDposClient(Interface* _c)
{
    if (dynamic_cast<SHDpos*>(_c->sealEngine()))
        return &dynamic_cast<SHDposClient&>(*_c);
    throw InvalidSHDposSealEngine();
}

dev::bacd::SHDposClient::SHDposClient(ChainParams const& _params, int _networkID, p2p::Host& _host,
    std::shared_ptr<GasPricer> _gpForAdoption, boost::filesystem::path const& _dbPath,
    boost::filesystem::path const& _snapshotPath, WithExisting _forceAction,
    TransactionQueue::Limits const& _l)
  : Client(_params, _networkID, _host, _gpForAdoption, _dbPath, _snapshotPath, _forceAction, _l)
{
    // will throw if we're not an dpos seal engine.
    asDposClient(*this);
    m_params = _params;
    init(_host, _networkID);
    LOG(m_logger)<< "init the dposClient ...";
}

dev::bacd::SHDposClient::~SHDposClient() 
{
    // to wake up the thread from Client::doWork()
    m_signalled.notify_all();
    terminate();
}

SHDpos* dev::bacd::SHDposClient::dpos() const
{
    return dynamic_cast<SHDpos*>(Client::sealEngine());
}

void dev::bacd::SHDposClient::startSealing()
{
    setName("DposClient");
    Client::startSealing();
}

void dev::bacd::SHDposClient::doWork(bool _doWait)
{
    try{
        bool t = true;
        //compare_exchange_strong(T& expected, T val, ...)
        //比较本身值和expected, 如果相等者旧值=val, 如果不等 expected=旧值

		cerror << "SHDposClient::doWork  : " << m_needStateReset   << "  _dowait : " << _doWait ;

		if(m_syncBlockQueue.compare_exchange_strong(t, false))
            syncBlockQueue();


        if(m_needStateReset)
        {
			cerror << " :SHDposClient::doWork   resetState";
            resetState();
            m_needStateReset = false;
        }

        t = true;
        bool isSealed = false;
        DEV_READ_GUARDED(x_working)
            isSealed = m_working.isSealed();

        if(!isSealed && !isMajorSyncing() && !m_remoteWorking && m_syncTransactionQueue.compare_exchange_strong(t, false))
            syncTransactionQueue();

        tick();

        rejigSealing();

        callQueuedFunctions();

        DEV_READ_GUARDED(x_working)
            isSealed = m_working.isSealed();
        // If the block is sealed, we have to wait for it to tickle through the block queue
        // (which only signals as wanting to be synced if it is ready).
        if(!m_syncBlockQueue && !m_syncTransactionQueue && (_doWait || isSealed) && isWorking())
        {
            std::unique_lock<std::mutex> l(x_signalled);
            m_signalled.wait_for(l, chrono::milliseconds(dpos()->dposConfig().blockInterval));
        }
    }catch (const boost::exception &e){
        cwarn <<  boost::diagnostic_information(e);
    }catch(const std::exception &e){
        cwarn << e.what();
    }catch (...){
        cwarn << "unkown exception.";
    }

}



void dev::bacd::SHDposClient::getEletorsByNum(std::vector<Address>& _v, size_t _num, std::vector<Address> _vector) const
{
	Block _block = blockByNumber(LatestBlock);
	_block.mutableVote().getSortElectors(_v, _num, _vector);
}


dev::Secret dev::bacd::SHDposClient::getVarlitorSecret(Address const& _addr) const
{
	Block _block = blockByNumber(LatestBlock);
	return _block.mutableVote().getVarlitorSecret(_addr);
}

void dev::bacd::SHDposClient::printfElectors()
{
//	Block _block = blockByNumber(LatestBlock);
//	std::unordered_map<Address, u256> _m = _block.mutableVote().getElectors();
//	std::cout << BrcYellow " dpos electors:" << std::endl << "{" << std::endl;;
//    for (auto val : _m)
//    {
//		std::cout << "Address:" << val.first << " tickets:" << val.second << std::endl;
//    }
//	std::cout << std::endl << "}" << BrcReset << std::endl;
}

void dev::bacd::SHDposClient::rejigSealing()
{
    if(!m_wouldSeal)
        return;
    //打包出块验证 包括出块周期，出块时间，出块人验证
    uint64_t _time = utcTimeMilliSec();            //get the systemTime
    if(!isBlockSeal(_time))
    {
        return;
    }

    if((wouldSeal() || remoteActive()) && !isMajorSyncing())
    {
        if(sealEngine()->shouldSeal(this))
        {
            m_wouldButShouldnot = false;

            LOG(m_loggerDetail) << "Rejigging seal engine...";
            DEV_WRITE_GUARDED(x_working)
            {
                if(m_working.isSealed())
                {
//                    LOG(m_logger) << "Tried to seal sealed block...";
                    return;
                }
                // TODO is that needed? we have "Generating seal on" below
                LOG(m_loggerDetail) << "Starting to seal block #" << m_working.info().number();
                // input a seal time to contral the seal transation time
                m_working.commitToSeal(bc(), m_extraData, dpos()->dposConfig().blockInterval * 2 / 3);
                //try into next new epoch and check some about varlitor for SH-DPOS
                dpos()->tryElect(utcTimeMilliSec());

                //add SH-Dpos data
				BlockHeader _h;
				_h.setDposCurrVarlitors(dpos()->currVarlitors());
				m_working.setDposData(_h);
				
                LOG(m_loggerDetail) <<BrcYellow "seal block add dposData is ok" << BrcYellow;
				printfElectors();


            }
            DEV_READ_GUARDED(x_working)
            {
                DEV_WRITE_GUARDED(x_postSeal)
                    m_postSeal = m_working;
                m_sealingInfo = m_working.info();

                auto author = m_working.author();
                if(!m_params.m_miner_priv_keys.count(author)){
                    cerror << "not find author : " << author << "private key , please set private key.";
                    return;
                }
                else{
                    m_sealingInfo.sign_block(m_params.m_miner_priv_keys.at(author));
                }

            }
            //出块
            if(wouldSeal())
            {
                //调用父类接口 声明回调，提供证明后调用 保存在 m_onSealGenerated
                sealEngine()->onSealGenerated([=](bytes const& _header){
                    if(this->submitSealed(_header))
                    {
                        m_onBlockSealed(_header);
                    }
                    else
                        LOG(m_logger) << "Submitting block failed...";
                                              });
                ctrace << "Generating seal on " << m_sealingInfo.hash((IncludeSeal)(WithoutSeal | WithoutSign)) << " #" << m_sealingInfo.number();
                sealEngine()->generateSeal(m_sealingInfo);
            }
        }
        else
            m_wouldButShouldnot = true;
    }
	if(!m_wouldSeal)
	{
		sealEngine()->cancelGeneration();
	}
}

void dev::bacd::SHDposClient::init(p2p::Host & _host, int _netWorkId)
{
    //about SH-dpos net_host CapabilityHostFace 接口
	cdebug << "capabilityHost :: SHDposHostCapability";
	auto brcCapability = make_shared<SHDposHostcapality>(_host.capabilityHost(),
							_netWorkId,
							[this](NodeID _nodeid, unsigned _id, RLP const& _r){
								dpos()->onDposMsg(_nodeid, _id, _r);
							},
							[this](NodeID const& _nodeid, u256 const& _peerCapabilityVersion){
								dpos()->requestStatus(_nodeid, _peerCapabilityVersion);
							});
	_host.registerCapability(brcCapability);
	dpos()->initNet(brcCapability);
    dpos()->initConfigAndGenesis(m_params);
    dpos()->setDposClient(this);
	m_bq.setOnBad([this](Exception& ex){ this->importBadBlock(ex); });
}

bool dev::bacd::SHDposClient::isBlockSeal(uint64_t _now)
{
    //LOG(m_logger) << "start to check time and varlitor ...";
    //验证时间 考虑创世区块时间
    if(!dpos()->checkDeadline(_now))
        return false;

    //验证出块人，周期
    if(!dpos()->isBolckSeal(_now))
        return false;
    return true;
}

void dev::bacd::SHDposClient::importBadBlock(Exception& _ex) const
{
	// BAD BLOCK!!!
	bytes const* block = boost::get_error_info<errinfo_block>(_ex);
	if(!block)
	{
		cwarn << "ODD: onBadBlock called but exception (" << _ex.what() << ") has no block in it.";
		cwarn << boost::diagnostic_information(_ex);
		return;
	}
    // SH-DPOS to del bad Block the must be Varlitor
	std::vector<Address> _varlitor = getCurrHeader().dposCurrVarlitors();
	auto ret = find(_varlitor.begin(), _varlitor.end(), author());
    if(ret != _varlitor.end())
	    dpos()->dellImportBadBlock(*block);
	badBlock(*block, _ex.what());
}


//void dev::bacd::SHDposClient::stopSealing() {
//    Worker::stopWorking();
//}
