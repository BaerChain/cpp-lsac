#include "BrchashClient.h"
#include "Brchash.h"
#include <boost/filesystem/path.hpp>
using namespace std;
using namespace dev;
using namespace dev::brc;
using namespace p2p;
namespace fs = boost::filesystem;

BrchashClient& dev::brc::asBrchashClient(Interface& _c)
{
    if (dynamic_cast<Brchash*>(_c.sealEngine()))
        return dynamic_cast<BrchashClient&>(_c);
    throw InvalidSealEngine();
}

BrchashClient* dev::brc::asBrchashClient(Interface* _c)
{
    if (dynamic_cast<Brchash*>(_c->sealEngine()))
        return &dynamic_cast<BrchashClient&>(*_c);
    throw InvalidSealEngine();
}

DEV_SIMPLE_EXCEPTION(ChainParamsNotBrchash);

BrchashClient::BrchashClient(ChainParams const& _params, int _networkID, p2p::Host& _host,
    std::shared_ptr<GasPricer> _gpForAdoption, fs::path const& _dbPath,
    fs::path const& _snapshotPath, WithExisting _forceAction,
    TransactionQueue::Limits const& _limits)
  : Client(
        _params, _networkID, _host, _gpForAdoption, _dbPath, _snapshotPath, _forceAction, _limits)
{
    // will throw if we're not an Brchash seal engine.
    asBrchashClient(*this);
}

BrchashClient::~BrchashClient()
{
    m_signalled.notify_all(); // to wake up the thread from Client::doWork()
    terminate();
}

Brchash* BrchashClient::brcash() const
{
    return dynamic_cast<Brchash*>(Client::sealEngine());
}

bool BrchashClient::isMining() const
{
    return brcash()->farm().isMining();
}

WorkingProgress BrchashClient::miningProgress() const
{
    if (isMining())
        return brcash()->farm().miningProgress();
    return WorkingProgress();
}

u256 BrchashClient::hashrate() const
{
    u256 r = externalHashrate();
    if (isMining())
        r += miningProgress().rate();
    return r;
}

std::tuple<h256, h256, h256> BrchashClient::getBrchashWork()
{
    // lock the work so a later submission isn't invalidated by processing a transaction elsewhere.
    // this will be reset as soon as a new block arrives, allowing more transactions to be processed.
    bool oldShould = shouldServeWork();
    m_lastGetWork = chrono::system_clock::now();

    if (!sealEngine()->shouldSeal(this))
        return std::tuple<h256, h256, h256>();

    // if this request has made us bother to serve work, prep it now.
    if (!oldShould && shouldServeWork())
        onPostStateChanged();
    else
        // otherwise, set this to true so that it gets prepped next time.
        m_remoteWorking = true;
    brcash()->manuallySetWork(m_sealingInfo);
    return std::tuple<h256, h256, h256>(m_sealingInfo.hash(WithoutSeal), Brchash::seedHash(m_sealingInfo), Brchash::boundary(m_sealingInfo));
}

bool BrchashClient::submitBrchashWork(h256 const& _mixHash, h64 const& _nonce)
{
    brcash()->manuallySubmitWork(_mixHash, _nonce);
    return true;
}

void BrchashClient::submitExternalHashrate(u256 const& _rate, h256 const& _id)
{
    WriteGuard writeGuard(x_externalRates);
    m_externalRates[_id] = make_pair(_rate, chrono::steady_clock::now());
}

u256 BrchashClient::externalHashrate() const
{
    u256 ret = 0;
    WriteGuard writeGuard(x_externalRates);
    for (auto i = m_externalRates.begin(); i != m_externalRates.end();)
        if (chrono::steady_clock::now() - i->second.second > chrono::seconds(5))
            i = m_externalRates.erase(i);
        else
            ret += i++->second.first;
    return ret;
}
