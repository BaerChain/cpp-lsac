/*
    DposClient.h
    管理调用 Dpos.h 的接口
    指定用户轮流出块，必须保证 配置验证用户能正常出块 否则会等待
 */
#pragma once
#include <libp2p/Host.h>
#include "ChainParams.h"
#include <libbrccore/KeyManager.h>
#include <libbrcdchain/Client.h>
#include <boost/filesystem/path.hpp>
#include "libbrcdchain/Interface.h"
#include "Common.h"
#include "libbrcdchain/Nodemonitor.h"
#include <libbrcdchain/Account.h>

namespace dev
{
namespace bacd
{
class SHDpos;
using namespace dev::brc;
DEV_SIMPLE_EXCEPTION(InvalidSHDposSealEngine);

class SHDposClient : public Client
{
public:
    SHDposClient(ChainParams const& _params, int _networkID, p2p::Host& _host,
        std::shared_ptr<GasPricer> _gpForAdoption, boost::filesystem::path const& _dbPath = {},
        boost::filesystem::path const& _snapshotPath = {},
        WithExisting _forceAction = WithExisting::Trust,
        int64_t _rebuild_num = 0,
        TransactionQueue::Limits const& _l = TransactionQueue::Limits{102400, 102400});

    ~SHDposClient();

public:
    SHDpos* dpos() const;
    void startSealing() override;
    void doWork(bool _doWait) override;
    int chainId() const override;

    inline const BlockHeader    getCurrHeader()const     { return m_bc.info(); }
    inline h256                 getCurrBlockhash()const  { return m_bc.currentHash(); }
    inline h256                 getGenesisHash()const    { return m_bc.genesisHash(); }
    inline BlockHeader const&   getGenesisHeader()const  { return m_bc.genesis(); }
    inline h256s                getTransationsHashByBlockNum(size_t num) const { return transactionHashes(hashFromNumber(num)); }

	void getEletorsByNum(std::vector<Address>& _v, size_t _num, std::vector<Address> _vector = std::vector<Address>()) const;
	void getCurrCreater(CreaterType _type, std::vector<Address>& _creaters) const;
	Secret getVarlitorSecret(Address const& _addr) const;

	bool verifyVarlitorPrivatrKey() const;
	bool verifyVarlitorPrivatrKey(Address const& _addr) const;

	/// verify standby_node
    ///@return true if own_addr can create_block
    ///@param super_addr : offline super_node addr
	bool  verify_standby(int64_t block_time, const dev::Address &own_addr, uint32_t varlitorInterval_time) const;

protected:
    void rejigSealing();
	void syncTransactionQueue();
private:
    void init(p2p::Host & _host, int _netWorkId);
    bool isBlockSeal(uint64_t _now);
	/// Called when we have attempted to import a bad block.
   /// @warning May be called from any thread.
	void importBadBlock(Exception& _ex) const;

    /// check the block is follow SHDpos  in mine creater_time
    /// @paramer _ph : the last block in chain
	bool checkPreviousBlock(BlockHeader const& _ph) const;
    void sendDataToNodeMonitor();
private:
    ChainParams                     m_params;          //配置
    Logger                          m_logger{createLogger(VerbosityInfo, "DposClinet")};

	int64_t                         m_startSeal_time =0;

    p2p::Host                       &m_p2pHost;
    NodeMonitor                     m_nodemonitor;
    bool                            m_is_firt_run = true;
};

SHDposClient& asDposClient(Interface& _c);
SHDposClient* asDposClient(Interface* _c);

}  // namespace brc
}  // namespace dev
