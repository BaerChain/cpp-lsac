#pragma once

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>

#include <libbrccore/Exceptions.h>

class BadArgument: public dev::Exception {};

class MinerCLI
{
public:
    enum class OperationMode
    {
        None,
        Benchmark
    };

    explicit MinerCLI(OperationMode _mode = OperationMode::None) : mode(_mode)
    {
    }

    void execute();

    std::string minerType() const { return m_minerType; }

    void interpretOptions(boost::program_options::variables_map const& _options);
    static boost::program_options::options_description createProgramOptions(unsigned int _lineLength);

private:
    void doBenchmark(std::string const& _m, unsigned _warmupDuration = 15, unsigned _trialDuration = 3, unsigned _trials = 5);

    /// Operating mode.
    OperationMode mode;

    /// Mining options
    std::string m_minerType = "cpu";
    unsigned m_miningThreads = UINT_MAX;
    uint64_t m_currentBlock = 0;

    /// Benchmarking params
    unsigned m_benchmarkWarmup = 3;
    unsigned m_benchmarkTrial = 3;
    unsigned m_benchmarkTrials = 5;
};
