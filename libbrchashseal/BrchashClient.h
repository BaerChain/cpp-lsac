#pragma once

#include <libbrcdchain/Client.h>
#include <boost/filesystem/path.hpp>
#include <tuple>

namespace dev
{
namespace brc
{

class Brchash;

DEV_SIMPLE_EXCEPTION(InvalidSealEngine);

class BrchashClient: public Client
{
public:
    /// Trivial forwarding constructor.
    BrchashClient(ChainParams const& _params, int _networkID, p2p::Host& _host,
        std::shared_ptr<GasPricer> _gpForAdoption, boost::filesystem::path const& _dbPath = {},
        boost::filesystem::path const& _snapshotPath = {},
        WithExisting _forceAction = WithExisting::Trust,
        TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024});
    ~BrchashClient();

    Brchash* brcash() const;

    /// Are we mining now?
    bool isMining() const;

    /// The hashrate...
    u256 hashrate() const;

    /// Check the progress of the mining.
    WorkingProgress miningProgress() const;

    /// @returns true only if it's worth bothering to prep the mining block.
    bool shouldServeWork() const { return m_bq.items().first == 0 && (isMining() || remoteActive()); }

    /// Update to the latest transactions and get hash of the current block to be mined minus the
    /// nonce (the 'work hash') and the difficulty to be met.
    /// @returns Tuple of hash without seal, seed hash, target boundary.
    std::tuple<h256, h256, h256> getBrchashWork();

    /** @brief Submit the proof for the proof-of-work.
     * @param _s A valid solution.
     * @return true if the solution was indeed valid and accepted.
     */
    bool submitBrchashWork(h256 const& _mixHash, h64 const& _nonce);

    void submitExternalHashrate(u256 const& _rate, h256 const& _id);

protected:
    u256 externalHashrate() const;

    // external hashrate
    mutable std::unordered_map<h256, std::pair<u256, std::chrono::steady_clock::time_point>> m_externalRates;
    mutable SharedMutex x_externalRates;
};

BrchashClient& asBrchashClient(Interface& _c);
BrchashClient* asBrchashClient(Interface* _c);

}
}
