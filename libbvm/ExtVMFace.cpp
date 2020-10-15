// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "ExtVMFace.h"

#include <bvmc/helpers.h>

namespace dev
{
namespace brc
{
static_assert(sizeof(Address) == sizeof(bvmc_address), "Address types size mismatch");
static_assert(alignof(Address) == alignof(bvmc_address), "Address types alignment mismatch");
static_assert(sizeof(h256) == sizeof(bvmc_uint256be), "Hash types size mismatch");
static_assert(alignof(h256) == alignof(bvmc_uint256be), "Hash types alignment mismatch");

bool EvmCHost::account_exists(bvmc::address const& _addr) const noexcept
{
    return m_extVM.exists(fromBvmC(_addr));
}

bvmc::bytes32 EvmCHost::get_storage(bvmc::address const& _addr, bvmc::bytes32 const& _key) const
    noexcept
{
    (void)_addr;
    assert(fromBvmC(_addr) == m_extVM.myAddress);
    return toBvmC(m_extVM.store(fromBvmC(_key)));
}

bvmc_storage_status EvmCHost::set_storage(
    bvmc::address const& _addr, bvmc::bytes32 const& _key, bvmc::bytes32 const& _value) noexcept
{
    (void)_addr;
    assert(fromBvmC(_addr) == m_extVM.myAddress);
    u256 const index = fromBvmC(_key);
    u256 const newValue = fromBvmC(_value);
    u256 const currentValue = m_extVM.store(index);

    if (newValue == currentValue)
        return BVMC_STORAGE_UNCHANGED;

    BRCSchedule const& schedule = m_extVM.brcSchedule();
    auto status = BVMC_STORAGE_MODIFIED;
    u256 const originalValue = m_extVM.originalStorageValue(index);
    if (originalValue == currentValue || !schedule.eip1283Mode)
    {
        if (currentValue == 0)
            status = BVMC_STORAGE_ADDED;
        else if (newValue == 0)
        {
            status = BVMC_STORAGE_DELETED;
            m_extVM.sub.refunds += schedule.sstoreRefundGas;
        }
    }
    else
    {
        status = BVMC_STORAGE_MODIFIED_AGAIN;
        if (originalValue != 0)
        {
            if (currentValue == 0)
                m_extVM.sub.refunds -= schedule.sstoreRefundGas;  // Can go negative.
            if (newValue == 0)
                m_extVM.sub.refunds += schedule.sstoreRefundGas;
        }
        if (originalValue == newValue)
        {
            if (originalValue == 0)
                m_extVM.sub.refunds += schedule.sstoreSetGas - schedule.sstoreUnchangedGas;
            else
                m_extVM.sub.refunds += schedule.sstoreResetGas - schedule.sstoreUnchangedGas;
        }
    }

    m_extVM.setStore(index, newValue);  // Interface uses native endianness

    return status;
}

bvmc::uint256be EvmCHost::get_balance(bvmc::address const& _addr) const noexcept
{
    return toBvmC(m_extVM.balance(fromBvmC(_addr)));
}

size_t EvmCHost::get_code_size(bvmc::address const& _addr) const noexcept
{
    return m_extVM.codeSizeAt(fromBvmC(_addr));
}

bvmc::bytes32 EvmCHost::get_code_hash(bvmc::address const& _addr) const noexcept
{
    return toBvmC(m_extVM.codeHashAt(fromBvmC(_addr)));
}

size_t EvmCHost::copy_code(bvmc::address const& _addr, size_t _codeOffset, byte* _bufferData,
    size_t _bufferSize) const noexcept
{
    Address addr = fromBvmC(_addr);
    bytes const& c = m_extVM.codeAt(addr);

    // Handle "big offset" edge case.
    if (_codeOffset >= c.size())
        return 0;

    size_t maxToCopy = c.size() - _codeOffset;
    size_t numToCopy = std::min(maxToCopy, _bufferSize);
    std::copy_n(&c[_codeOffset], numToCopy, _bufferData);
    return numToCopy;
}

void EvmCHost::selfdestruct(bvmc::address const& _addr, bvmc::address const& _beneficiary) noexcept
{
    (void)_addr;
    assert(fromBvmC(_addr) == m_extVM.myAddress);
    m_extVM.selfdestruct(fromBvmC(_beneficiary));
}


void EvmCHost::emit_log(bvmc::address const& _addr, uint8_t const* _data, size_t _dataSize,
    bvmc::bytes32 const _topics[], size_t _numTopics) noexcept
{
    (void)_addr;
    assert(fromBvmC(_addr) == m_extVM.myAddress);
    h256 const* pTopics = reinterpret_cast<h256 const*>(_topics);
    m_extVM.log(h256s{pTopics, pTopics + _numTopics}, bytesConstRef{_data, _dataSize});
}

bvmc_tx_context EvmCHost::get_tx_context() const noexcept
{
    bvmc_tx_context result = {};
    result.tx_gas_price = toBvmC(m_extVM.gasPrice);
    result.tx_origin = toBvmC(m_extVM.origin);

    auto const& envInfo = m_extVM.envInfo();
    result.block_coinbase = toBvmC(envInfo.author());
    result.block_number = envInfo.number();
    result.block_timestamp = envInfo.timestamp();
    result.block_gas_limit = static_cast<int64_t>(envInfo.gasLimit());
    result.block_difficulty = toBvmC(envInfo.difficulty());
    result.chain_id = toBvmC(envInfo.chainID());
    return result;
}

bvmc::bytes32 EvmCHost::get_block_hash(int64_t _number) const noexcept
{
    return toBvmC(m_extVM.blockHash(_number));
}

bvmc::result EvmCHost::create(bvmc_message const& _msg) noexcept
{
    u256 gas = _msg.gas;
    u256 value = fromBvmC(_msg.value);
    bytesConstRef init = {_msg.input_data, _msg.input_size};
    u256 salt = fromBvmC(_msg.create2_salt);
    Instruction opcode = _msg.kind == BVMC_CREATE ? Instruction::CREATE : Instruction::CREATE2;

    // ExtVM::create takes the sender address from .myAddress.
    assert(fromBvmC(_msg.sender) == m_extVM.myAddress);

    CreateResult result = m_extVM.create(value, gas, init, opcode, salt, {});
    bvmc_result bvmcResult = {};
    bvmcResult.status_code = result.status;
    bvmcResult.gas_left = static_cast<int64_t>(gas);

    if (result.status == BVMC_SUCCESS)
        bvmcResult.create_address = toBvmC(result.address);
    else
    {
        // Pass the output to the EVM without a copy. The EVM will delete it
        // when finished with it.

        // First assign reference. References are not invalidated when vector
        // of bytes is moved. See `.takeBytes()` below.
        bvmcResult.output_data = result.output.data();
        bvmcResult.output_size = result.output.size();

        // Place a new vector of bytes containing output in result's reserved memory.
        auto* data = bvmc_get_optional_storage(&bvmcResult);
        static_assert(sizeof(bytes) <= sizeof(*data), "Vector is too big");
        new (data) bytes(result.output.takeBytes());
        // Set the destructor to delete the vector.
        bvmcResult.release = [](bvmc_result const* _result) {
            auto* data = bvmc_get_const_optional_storage(_result);
            auto& output = reinterpret_cast<bytes const&>(*data);
            // Explicitly call vector's destructor to release its data.
            // This is normal pattern when placement new operator is used.
            output.~bytes();
        };
    }
    return bvmc::result{bvmcResult};
}

bvmc::result EvmCHost::call(bvmc_message const& _msg) noexcept
{
    assert(_msg.gas >= 0 && "Invalid gas value");
    assert(_msg.depth == static_cast<int>(m_extVM.depth) + 1);

    // Handle CREATE separately.
    if (_msg.kind == BVMC_CREATE || _msg.kind == BVMC_CREATE2)
        return create(_msg);

    CallParameters params;
    params.gas = _msg.gas;
    params.apparentValue = fromBvmC(_msg.value);
    params.valueTransfer = _msg.kind == BVMC_DELEGATECALL ? 0 : params.apparentValue;
    params.senderAddress = fromBvmC(_msg.sender);
    params.codeAddress = fromBvmC(_msg.destination);
    params.receiveAddress = _msg.kind == BVMC_CALL ? params.codeAddress : m_extVM.myAddress;
    params.data = {_msg.input_data, _msg.input_size};
    params.staticCall = (_msg.flags & BVMC_STATIC) != 0;
    params.onOp = {};

    CallResult result = m_extVM.call(params);
    bvmc_result bvmcResult = {};
    bvmcResult.status_code = result.status;
    bvmcResult.gas_left = static_cast<int64_t>(params.gas);

    // Pass the output to the EVM without a copy. The EVM will delete it
    // when finished with it.

    // First assign reference. References are not invalidated when vector
    // of bytes is moved. See `.takeBytes()` below.
    bvmcResult.output_data = result.output.data();
    bvmcResult.output_size = result.output.size();

    // Place a new vector of bytes containing output in result's reserved memory.
    auto* data = bvmc_get_optional_storage(&bvmcResult);
    static_assert(sizeof(bytes) <= sizeof(*data), "Vector is too big");
    new (data) bytes(result.output.takeBytes());
    // Set the destructor to delete the vector.
    bvmcResult.release = [](bvmc_result const* _result) {
        auto* data = bvmc_get_const_optional_storage(_result);
        auto& output = reinterpret_cast<bytes const&>(*data);
        // Explicitly call vector's destructor to release its data.
        // This is normal pattern when placement new operator is used.
        output.~bytes();
    };
    return bvmc::result{bvmcResult};
}

ExtVMFace::ExtVMFace(EnvInfo const& _envInfo, Address _myAddress, Address _caller, Address _origin,
    u256 _value, u256 _gasPrice, bytesConstRef _data, bytes _code, h256 const& _codeHash,
    u256 const& _version, unsigned _depth, bool _isCreate, bool _staticCall)
  : m_envInfo(_envInfo),
    myAddress(_myAddress),
    caller(_caller),
    origin(_origin),
    value(_value),
    gasPrice(_gasPrice),
    data(_data),
    code(std::move(_code)),
    codeHash(_codeHash),
    version(_version),
    depth(_depth),
    isCreate(_isCreate),
    staticCall(_staticCall)
{}

}  // namespace brc
}  // namespace dev
