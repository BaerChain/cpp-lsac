#pragma once

#include <tuple>
#include <libbrcdchain/Client.h>
#include <boost/filesystem/path.hpp>

namespace dev
{
namespace brc
{

DEV_SIMPLE_EXCEPTION(ChainParamsInvalid);
DEV_SIMPLE_EXCEPTION(ImportBlockFailed);

class ClientTest: public Client
{
public:
    /// Trivial forwarding constructor.
    ClientTest(ChainParams const& _params, int _networkID, p2p::Host& _host,
        std::shared_ptr<GasPricer> _gpForAdoption,
        boost::filesystem::path const& _dbPath = boost::filesystem::path(),
        WithExisting _forceAction = WithExisting::Trust,
        TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024});
    ~ClientTest();

    void setChainParams(std::string const& _genesis);
    bool mineBlocks(unsigned _count) noexcept;
    void modifyTimestamp(int64_t _timestamp);
    void rewindToBlock(unsigned _number);
    h256 importRawBlock(std::string const& _blockRLP);
    bool completeSync();

protected:
    unsigned const m_singleBlockMaxMiningTimeInSeconds = 5;
};

ClientTest& asClientTest(Interface& _c);
ClientTest* asClientTest(Interface* _c);

}
}
