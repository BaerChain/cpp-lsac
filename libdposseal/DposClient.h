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

    //dpos消息 操作所有是自己，目标是 _targe_address
	bool applyDposMassage(DposPacketType _type, Address _targe_address);

protected:
    void rejigSealing();

private:
	void init(p2p::Host& _host, int _netWorkId);

	bool isBlockSeal(uint64_t _now);

private:
    int m_timespace;  //出块时间间隔 毫秒
    //上次出块时间
    mutable std::chrono::system_clock::time_point m_lastBlock = std::chrono::system_clock::now();

    ChainParams m_params;          //配置
};

DposClient& asDposClient(Interface& _c);
DposClient* asDposClient(Interface* _c);

}  // namespace eth
}  // namespace dev