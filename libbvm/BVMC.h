#pragma once

#include <libbvm/VMFace.h>

#include <bvmc/bvmc.h>
#include <bvmc/helpers.h>

namespace dev
{
namespace brc
{

/// The RAII wrapper for an BVMC instance.
class BRC
{
public:
    explicit BRC(bvmc_instance* _instance) noexcept;

    ~BRC() { bvmc_destroy(m_instance); }

    BRC(BRC const&) = delete;
    BRC& operator=(BRC) = delete;

    char const* name() const noexcept { return bvmc_vm_name(m_instance); }

    char const* version() const noexcept { return bvmc_vm_version(m_instance); }

    class Result
    {
    public:
        explicit Result(bvmc_result const& _result):
            m_result(_result)
        {}

        ~Result()
        {
            if (m_result.release)
                m_result.release(&m_result);
        }

        Result(Result&& _other) noexcept:
            m_result(_other.m_result)
        {
            // Disable releaser of the rvalue object.
            _other.m_result.release = nullptr;
        }

        Result(Result const&) = delete;
        Result& operator=(Result const&) = delete;

        bvmc_status_code status() const
        {
            return m_result.status_code;
        }

        int64_t gasLeft() const
        {
            return m_result.gas_left;
        }

        bytesConstRef output() const
        {
            return {m_result.output_data, m_result.output_size};
        }

    private:
        bvmc_result m_result;
    };

    /// Handy wrapper for bvmc_execute().
    Result execute(ExtVMFace& _ext, int64_t gas);

    /// Translate the BRCSchedule to BVMC revision.
    static bvmc_revision toRevision(BRCSchedule const& _schedule);

private:
    /// The VM instance created with BVMC <prefix>_create() function.
    bvmc_instance* m_instance = nullptr;
};


/// The wrapper implementing the VMFace interface with a BVMC VM as a backend.
class BVMC : public BRC, public VMFace
{
public:
    explicit BVMC(bvmc_instance* _instance) : BRC(_instance) {}

    owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) final;
};
}
}
