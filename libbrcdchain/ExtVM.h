#pragma once

#include "Executive.h"
#include "State.h"

#include <libbrccore/Common.h>
#include <libbrccore/SealEngine.h>
#include <libbvm/ExtVMFace.h>

#include <functional>
#include <map>

namespace dev
{
namespace brc
{

class SealEngineFace;

/// Externality interface for the Virtual Machine providing access to world state.
class ExtVM : public ExtVMFace
{
public:
    /// Full constructor.
    ExtVM(State& _s, EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, Address _myAddress,
        Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data,
        bytesConstRef _code, h256 const& _codeHash, unsigned _depth, bool _isCreate,
        bool _staticCall)
      : ExtVMFace(_envInfo, _myAddress, _caller, _origin, _value, _gasPrice, _data, _code.toBytes(),
            _codeHash, 0, _depth, _isCreate, _staticCall),
        m_s(_s),
        m_sealEngine(_sealEngine),
        m_brcSchedule(m_sealEngine.brcSchedule(envInfo().number()))
    {
        // Contract: processing account must exist. In case of CALL, the ExtVM
        // is created only if an account has code (so exist). In case of CREATE
        // the account must be created first.
        assert(m_s.addressInUse(_myAddress));
    }

    /// Read storage location.
    u256 store(u256 _n) final { return m_s.storage(myAddress, _n); }

    /// Write a value in storage.
    void setStore(u256 _n, u256 _v) final;

    /// Read original storage value (before modifications in the current transaction).
    u256 originalStorageValue(u256 const& _key) final
    {
        return m_s.originalStorageValue(myAddress, _key);
    }

    /// Read address's code.
    bytes const& codeAt(Address _a) final { return m_s.code(_a); }

    /// @returns the size of the code in  bytes at the given address.
    size_t codeSizeAt(Address _a) final;

    /// @returns the hash of the code at the given address.
    h256 codeHashAt(Address _a) final;

    /// Create a new contract.
    CreateResult create(u256 _endowment, u256& io_gas, bytesConstRef _code, Instruction _op, u256 _salt, OnOpFunc const& _onOp = {}) final;

    /// Create a new message call.
    CallResult call(CallParameters& _params) final;

    /// Read address's balance.
    u256 balance(Address _a) final { return m_s.BRC(_a); }

    /// Does the account exist?
    bool exists(Address _a) final
    {
        if (brcSchedule().emptinessIsNonexistence())
            return m_s.accountNonemptyAndExisting(_a);
        else
            return m_s.addressInUse(_a);
    }

    /// Suicide the associated contract to the given address.
    // void suicide(Address _a) final;
    void selfdestruct(Address _a) final;
    
    /// Return the BRC gas-price schedule for this execution context.
    BRCSchedule const& brcSchedule() const final
    {
        return m_sealEngine.brcSchedule(envInfo().number());
    }

    State const& state() const { return m_s; }

    /// Hash of a block if within the last 256 blocks, or h256() otherwise.
    h256 blockHash(u256 _number) final;

private:
    State& m_s;  ///< A reference to the base state.
    SealEngineFace const& m_sealEngine;
    BRCSchedule const& m_brcSchedule;
};

}
}

