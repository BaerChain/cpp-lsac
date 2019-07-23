#include "BrchashCPUMiner.h"
#include "Brchash.h"

#include <brcash/brcash.hpp>

#include <thread>
#include <chrono>
#include <random>

using namespace std;
using namespace dev;
using namespace brc;

unsigned BrchashCPUMiner::s_numInstances = 0;


BrchashCPUMiner::BrchashCPUMiner(GenericMiner<BrchashProofOfWork>::ConstructionInfo const& _ci)
  : GenericMiner<BrchashProofOfWork>(_ci)
{
}

BrchashCPUMiner::~BrchashCPUMiner()
{
    stopWorking();
}

void BrchashCPUMiner::kickOff()
{
    stopWorking();
    startWorking();
}

void BrchashCPUMiner::pause()
{
    stopWorking();
}

void BrchashCPUMiner::startWorking()
{
    if (!m_thread)
    {
        m_shouldStop = false;
        m_thread.reset(new thread(&BrchashCPUMiner::minerBody, this));
    }
}

void BrchashCPUMiner::stopWorking()
{
    if (m_thread)
    {
        m_shouldStop = true;
        m_thread->join();
        m_thread.reset();
    }
}


void BrchashCPUMiner::minerBody()
{
    setThreadName("miner" + toString(index()));

    auto tid = std::this_thread::get_id();
    static std::mt19937_64 s_eng((utcTime() + std::hash<decltype(tid)>()(tid)));

    uint64_t tryNonce = s_eng();

    // FIXME: Use epoch number, not seed hash in the work package.
    WorkPackage w = work();

    int epoch = brcash::find_epoch_number(toBrchash(w.seedHash));
    auto& brchashContext = brcash::get_global_epoch_context_full(epoch);

    h256 boundary = w.boundary;
    for (unsigned hashCount = 1; !m_shouldStop; tryNonce++, hashCount++)
    {
        auto result = brcash::hash(brchashContext, toBrchash(w.headerHash()), tryNonce);
        h256 value = h256(result.final_hash.bytes, h256::ConstructFromPointer);
        if (value <= boundary && submitProof(BrchashProofOfWork::Solution{(h64)(u64)tryNonce,
                                     h256(result.mix_hash.bytes, h256::ConstructFromPointer)}))
            break;
        if (!(hashCount % 100))
            accumulateHashes(100);
    }
}

std::string BrchashCPUMiner::platformInfo()
{
    string baseline = toString(std::thread::hardware_concurrency()) + "-thread CPU";
    return baseline;
}
