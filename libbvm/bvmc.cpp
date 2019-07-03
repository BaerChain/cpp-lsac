#include "BVMC.h"

#include <libdevcore/Log.h>
#include <libbvm/VMFactory.h>

namespace dev
{
namespace brc
{
BRC::BRC(bvmc_instance* _instance) noexcept : m_instance(_instance)
{
    assert(m_instance != nullptr);
    assert(bvmc_is_abi_compatible(m_instance));

    // Set the options.
    for (auto& pair : bvmcOptions())
    {
        auto result = bvmc_set_option(m_instance, pair.first.c_str(), pair.second.c_str());
        switch (result)
        {
        case BVMC_SET_OPTION_SUCCESS:
            break;
        case BVMC_SET_OPTION_INVALID_NAME:
            cwarn << "Unknown BVMC option '" << pair.first << "'";
            break;
        case BVMC_SET_OPTION_INVALID_VALUE:
            cwarn << "Invalid value '" << pair.second << "' for BVMC option '" << pair.first << "'";
            break;
        default:
            cwarn << "Unknown error when setting BVMC option '" << pair.first << "'";
        }
    }
}

/// Handy wrapper for bvmc_execute().
BRC::Result BRC::execute(ExtVMFace& _ext, int64_t gas)
{
    auto mode = toRevision(_ext.brcSchedule());
    bvmc_call_kind kind = _ext.isCreate ? BVMC_CREATE : BVMC_CALL;
    uint32_t flags = _ext.staticCall ? BVMC_STATIC : 0;
    assert(flags != BVMC_STATIC || kind == BVMC_CALL);  // STATIC implies a CALL.
    bvmc_message msg = {kind, flags, static_cast<int32_t>(_ext.depth), gas, toBvmC(_ext.myAddress),
        toBvmC(_ext.caller), _ext.data.data(), _ext.data.size(), toBvmC(_ext.value),
        toBvmC(0x0_cppui256)};
    return BRC::Result{
        bvmc_execute(m_instance, &_ext, mode, &msg, _ext.code.data(), _ext.code.size())};
}

owning_bytes_ref BVMC::exec(u256& io_gas, ExtVMFace& _ext, const OnOpFunc& _onOp)
{
    assert(_ext.envInfo().number() >= 0);
    assert(_ext.envInfo().timestamp() >= 0);

    constexpr int64_t int64max = std::numeric_limits<int64_t>::max();

    // TODO: The following checks should be removed by changing the types
    //       used for gas, block number and timestamp.
    (void)int64max;
    assert(io_gas <= int64max);
    assert(_ext.envInfo().gasLimit() <= int64max);
    assert(_ext.depth <= static_cast<size_t>(std::numeric_limits<int32_t>::max()));

    auto gas = static_cast<int64_t>(io_gas);
    BRC::Result r = execute(_ext, gas);

    switch (r.status())
    {
    case BVMC_SUCCESS:
        io_gas = r.gasLeft();
        // FIXME: Copy the output for now, but copyless version possible.
        return {r.output().toVector(), 0, r.output().size()};

    case BVMC_REVERT:
        io_gas = r.gasLeft();
        // FIXME: Copy the output for now, but copyless version possible.
        throw RevertInstruction{{r.output().toVector(), 0, r.output().size()}};

    case BVMC_OUT_OF_GAS:
    case BVMC_FAILURE:
        BOOST_THROW_EXCEPTION(OutOfGas());

    case BVMC_INVALID_INSTRUCTION:  // NOTE: this could have its own exception
    case BVMC_UNDEFINED_INSTRUCTION:
        BOOST_THROW_EXCEPTION(BadInstruction());

    case BVMC_BAD_JUMP_DESTINATION:
        BOOST_THROW_EXCEPTION(BadJumpDestination());

    case BVMC_STACK_OVERFLOW:
        BOOST_THROW_EXCEPTION(OutOfStack());

    case BVMC_STACK_UNDERFLOW:
        BOOST_THROW_EXCEPTION(StackUnderflow());

    case BVMC_INVALID_MEMORY_ACCESS:
        BOOST_THROW_EXCEPTION(BufferOverrun());

    case BVMC_STATIC_MODE_VIOLATION:
        BOOST_THROW_EXCEPTION(DisallowedStateChange());

    case BVMC_REJECTED:
        cwarn << "Execution rejected by BVMC, executing with default VM implementation";
        return VMFactory::create(VMKind::Legacy)->exec(io_gas, _ext, _onOp);

    case BVMC_INTERNAL_ERROR:
    default:
        if (r.status() <= BVMC_INTERNAL_ERROR)
            BOOST_THROW_EXCEPTION(InternalVMError{} << errinfo_bvmcStatusCode(r.status()));
        else
            // These cases aren't really internal errors, just more specific
            // error codes returned by the VM. Map all of them to OOG.
            BOOST_THROW_EXCEPTION(OutOfGas());
    }
}

bvmc_revision BRC::toRevision(BRCSchedule const& _schedule)
{
    if (_schedule.haveCreate2)
        return BVMC_CONSTANTINOPLE;
    if (_schedule.haveRevert)
        return BVMC_BYZANTIUM;
    if (_schedule.eip158Mode)
        return BVMC_SPURIOUS_DRAGON;
    if (_schedule.eip150Mode)
        return BVMC_TANGERINE_WHISTLE;
    if (_schedule.haveDelegateCall)
        return BVMC_HOMESTEAD;
    return BVMC_FRONTIER;
}
}  // namespace brc
}  // namespace dev
