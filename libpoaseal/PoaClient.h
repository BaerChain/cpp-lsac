/*
    PoaClient.h
    管理调用 Poa.h 的接口
    POA 指定用户轮流出块，必须保证 配置验证用户能正常出块 否则会等待
 */
#pragma once
#include "Poa.h"
#include <libp2p/Host.h>
#include <libbrccore/KeyManager.h>
#include <libbrcdchain/Client.h>
#include <boost/filesystem/path.hpp>

namespace dev
{
namespace brc
{
class Poa;
DEV_SIMPLE_EXCEPTION(InvalidPoaSealEngine);

class PoaClient : public Client
{
public:
    PoaClient(ChainParams const& _params, int _networkID, p2p::Host& _host,
        std::shared_ptr<GasPricer> _gpForAdoption, boost::filesystem::path const& _dbPath = {},
        boost::filesystem::path const& _snapshotPath = {},
        WithExisting _forceAction = WithExisting::Trust,
        TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024});

    ~PoaClient();

private:
	void init(p2p::Host& _host, int _networkID);

public:
    Poa* poa() const;
    void doWork(bool _doWait) override;
    void startSealing() override;
    void stopSealing() override;

    //设置撤销验证人
    bool setValidatorAccount(const std::string& _address, bool _ret);

protected:
    void rejigSealing();
    void syncBlockQueue();

private:
    bool setValidator(const std::string& _address, bool _ret);

private:
    mutable Mutex m_mutex; 
    int m_timespace;  //出块时间间隔 毫秒
    //上次出块时间
    mutable std::chrono::system_clock::time_point m_lastBlock = std::chrono::system_clock::now();
    ChainParams m_params;          //配置
    //std::vector<Address> m_poaValidatorAccount; // 验证人数组 按顺序出块

};

PoaClient& asPoaClient(Interface& _c);
PoaClient* asPoaClient(Interface* _c);

}  // namespace brc
}  // namespace dev