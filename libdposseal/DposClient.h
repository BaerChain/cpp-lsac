/*
    DposClient.h
    管理调用 Dpos.h 的接口
    指定用户轮流出块，必须保证 配置验证用户能正常出块 否则会等待
 */
#pragma once
#include <libp2p/Host.h>
#include <libethcore/KeyManager.h>
#include <libethereum/Client.h>
#include <boost/filesystem/path.hpp>
#include "libethereum/Interface.h"

namespace dev
{
namespace bacd
{
class Dpos;
using namespace dev::eth;
DEV_SIMPLE_EXCEPTION(InvalidDposSealEngine);

class DposClient : public Client
{
public:
    DposClient(ChainParams const& _params, int _networkID, p2p::Host& _host,
        std::shared_ptr<GasPricer> _gpForAdoption, boost::filesystem::path const& _dbPath = {},
        boost::filesystem::path const& _snapshotPath = {},
        WithExisting _forceAction = WithExisting::Trust,
        TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024});

    ~DposClient();

public:
    Dpos* dpos() const;
    void startSealing() override;
    void doWork(bool _doWait) override;

    inline const BlockHeader    getCurrHeader()const     { return m_bc.info(); }
    inline h256                 getCurrBlockhash()const  { return m_bc.currentHash(); }
    inline h256                 getGenesisHash()const    { return m_bc.genesisHash(); }
    inline BlockHeader const&   getGenesisHeader()const  { return m_bc.genesis(); }
    inline h256s                getTransationsHashByBlockNum(size_t num) const { return transactionHashes(hashFromNumber(num)); }

	void   getEletorsByNum(std::vector<Address>& _v, size_t _num) const;

	void   printfElectors();
	
protected:
    void rejigSealing();

private:
    void init(p2p::Host & _host, int _netWorkId);
    bool isBlockSeal(uint64_t _now);
private:
    ChainParams                     m_params;          //配置
    Logger                          m_logger{createLogger(VerbosityInfo, "DposClinet")};
};

DposClient& asDposClient(Interface& _c);
DposClient* asDposClient(Interface* _c);

}  // namespace eth
}  // namespace dev