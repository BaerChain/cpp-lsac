#include "MinerAux.h"

#include <boost/algorithm/string/case_conv.hpp>

#include <libdevcore/CommonJS.h>
#include <libbrccore/BasicAuthority.h>
#include <libbrchashseal/Brchash.h>
#include <libbrchashseal/BrchashCPUMiner.h>

using namespace std;
using namespace boost::program_options;
using namespace dev;
using namespace brc;
using namespace dev::brc;

void MinerCLI::interpretOptions(variables_map const& _options)
{
    if (_options.count("benchmark"))
        mode = OperationMode::Benchmark;

    if (_options.count("benchmark-warmup"))
        m_benchmarkWarmup = _options["benchmark-warmup"].as<int>();
    else
        m_benchmarkWarmup = 3;

    if (_options.count("benchmark-trial"))
        m_benchmarkTrial = _options["benchmark-trial"].as<int>();
    else
        m_benchmarkTrial = 3;

    if (_options.count("benchmark-trials"))
        m_benchmarkTrials = _options["benchmark-trials"].as<int>();
    else
        m_benchmarkTrials = 5;

    if (_options.count("cpu"))
        m_minerType = "cpu";

    if (_options.count("mining-threads"))
        m_miningThreads = _options["mining-threads"].as<unsigned int>();
    else
        m_miningThreads = UINT_MAX;

    if (_options.count("current-block"))
        m_currentBlock = _options["current-block"].as<unsigned int>();
    else
        m_currentBlock = 0;
}

options_description MinerCLI::createProgramOptions(unsigned int _lineLength)
{
    options_description benchmarkOptions("BENCHMARKING MODE", _lineLength);
    auto addBenchmarkOption = benchmarkOptions.add_options();

    addBenchmarkOption("benchmark,M", "Benchmark for mining and exit");
    addBenchmarkOption("benchmark-warmup", value<int>()->value_name("<seconds>"),
        "Set the duration of warmup for the benchmark tests (default: 3)");
    addBenchmarkOption("benchmark-trial", value<int>()->value_name("<seconds>"),
        "Set the duration for each trial for the benchmark tests (default: 3)");
    addBenchmarkOption("benchmark-trials", value<int>()->value_name("<n>"),
        "Set the number of trials for the benchmark tests (default: 5)");

    options_description miningOptions("MINING CONFIGURATION", _lineLength);
    auto addMiningOption = miningOptions.add_options();

    addMiningOption("cpu,C", "When mining, use the CPU");
    addMiningOption("mining-threads,t", value<unsigned int>()->value_name("<n>"),
        "Limit number of CPU/GPU miners to n (default: use everything available on selected platform)");
    addMiningOption("current-block", value<unsigned int>()->value_name("<n>"),
        "Let the miner know the current block number at configuration time. Will help determine DAG size and required GPU memory");
    addMiningOption("disable-submit-hashrate", "When mining, don't submit hashrate to node\n");

    return benchmarkOptions.add(miningOptions);
}

void MinerCLI::execute()
{
    if (m_minerType == "cpu")
        BrchashCPUMiner::setNumInstances(m_miningThreads);
    else if (mode == OperationMode::Benchmark)
        doBenchmark(m_minerType, m_benchmarkWarmup, m_benchmarkTrial, m_benchmarkTrials);
}

void MinerCLI::doBenchmark(std::string const& _m, unsigned _warmupDuration, unsigned _trialDuration, unsigned _trials)
{
    BlockHeader genesis;
    genesis.setDifficulty(1 << 18);
    cdebug << Brchash::boundary(genesis);

    GenericFarm<BrchashProofOfWork> f;
    map<string, GenericFarm<BrchashProofOfWork>::SealerDescriptor> sealers;
    sealers["cpu"] = GenericFarm<BrchashProofOfWork>::SealerDescriptor{&BrchashCPUMiner::instances, [](GenericMiner<BrchashProofOfWork>::ConstructionInfo ci){ return new BrchashCPUMiner(ci); }};
    f.setSealers(sealers);
    f.onSolutionFound([&](BrchashProofOfWork::Solution) { return false; });

    string platformInfo = BrchashCPUMiner::platformInfo();
    cout << "Benchmarking on platform: " << platformInfo << endl;

    genesis.setDifficulty(u256(1) << 63);
    f.setWork(genesis);
    f.start(_m);

    map<u256, WorkingProgress> results;
    u256 mean = 0;
    u256 innerMean = 0;
    for (unsigned i = 0; i <= _trials; ++i)
    {
        if (!i)
            cout << "Warming up..." << endl;
        else
            cout << "Trial " << i << "... " << flush;
        this_thread::sleep_for(chrono::seconds(i ? _trialDuration : _warmupDuration));

        auto mp = f.miningProgress();
        f.resetMiningProgress();
        if (!i)
            continue;
        auto rate = mp.rate();

        cout << rate << endl;
        results[rate] = mp;
        mean += rate;
    }
    f.stop();
    int j = -1;
    for (auto const& r: results)
        if (++j > 0 && j < (int)_trials - 1)
            innerMean += r.second.rate();
    innerMean /= (_trials - 2);
    cout << "min/mean/max: " << results.begin()->second.rate() << "/" << (mean / _trials) << "/" << results.rbegin()->second.rate() << " H/s" << endl;
    cout << "inner mean: " << innerMean << " H/s" << endl;
    exit(0);
}