
#pragma once

#include "BrchashProofOfWork.h"

#include <libbrcdchain/GenericMiner.h>

namespace dev
{
namespace brc
{
class BrchashCPUMiner : public GenericMiner<BrchashProofOfWork>
{
public:
    explicit BrchashCPUMiner(GenericMiner<BrchashProofOfWork>::ConstructionInfo const& _ci);
    ~BrchashCPUMiner() override;

    static unsigned instances()
    {
        return s_numInstances > 0 ? s_numInstances : std::thread::hardware_concurrency();
    }
    static std::string platformInfo();
    static void setNumInstances(unsigned _instances)
    {
        s_numInstances = std::min<unsigned>(_instances, std::thread::hardware_concurrency());
    }

protected:
    void kickOff() override;
    void pause() override;

private:
    static unsigned s_numInstances;

    void startWorking();
    void stopWorking();
    void minerBody();

    std::unique_ptr<std::thread> m_thread;
    std::atomic<bool> m_shouldStop;
};
}  // namespace brc
}  // namespace dev
