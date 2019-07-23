#include "ExtVMFace.h"

#include <bvmc/helpers.h>

namespace dev
{
namespace brc
{
namespace
{

static_assert(sizeof(Address) == sizeof(bvmc_address), "Address types size mismatch");
static_assert(alignof(Address) == alignof(bvmc_address), "Address types alignment mismatch");
static_assert(sizeof(h256) == sizeof(bvmc_uint256be), "Hash types size mismatch");
static_assert(alignof(h256) == alignof(bvmc_uint256be), "Hash types alignment mismatch");

bool accountExists(bvmc_context* _context, bvmc_address const* _addr) noexcept
{
    auto& env = static_cast<ExtVMFace&>(*_context);
    Address addr = fromBvmC(*_addr);
    return env.exists(addr);
}

bvmc_bytes32 getStorage(
    bvmc_context* _context, bvmc_address const* _addr, bvmc_uint256be const* _key) noexcept
{
    (void) _addr;
    auto& env = static_cast<ExtVMFace&>(*_context);
    assert(fromBvmC(*_addr) == env.myAddress);
    u256 key = fromBvmC(*_key);
    return toBvmC(env.store(key));
}

bvmc_storage_status setStorage(bvmc_context* _context, bvmc_address const* _addr,
    bvmc_uint256be const* _key, bvmc_uint256be const* _value) noexcept
{
    (void)_addr;
    auto& env = static_cast<ExtVMFace&>(*_context);
    assert(fromBvmC(*_addr) == env.myAddress);
    u256 const index = fromBvmC(*_key);
    u256 const newValue = fromBvmC(*_value);
    u256 const currentValue = env.store(index);

    if (newValue == currentValue)
        return BVMC_STORAGE_UNCHANGED;

    BRCSchedule const& schedule = env.brcSchedule();
    auto status = BVMC_STORAGE_MODIFIED;
    u256 const originalValue = env.originalStorageValue(index);
    if (originalValue == currentValue || !schedule.eip1283Mode)
    {
        if (currentValue == 0)
            status = BVMC_STORAGE_ADDED;
        else if (newValue == 0)
        {
            status = BVMC_STORAGE_DELETED;
            env.sub.refunds += schedule.sstoreRefundGas;
        }
    }
    else
    {
        status = BVMC_STORAGE_MODIFIED_AGAIN;
        if (originalValue != 0)
        {
            if (currentValue == 0)
                env.sub.refunds -= schedule.sstoreRefundGas;  // Can go negative.
            if (newValue == 0)
                env.sub.refunds += schedule.sstoreRefundGas;
        }
        if (originalValue == newValue)
        {
            if (originalValue == 0)
                env.sub.refunds += schedule.sstoreRefundGas + schedule.sstoreRefundNonzeroGas;
            else
                env.sub.refunds += schedule.sstoreRefundNonzeroGas;
        }
    }

    env.setStore(index, newValue);  // Interface uses native endianness

    return status;
}

bvmc_uint256be getBalance(bvmc_context* _context, bvmc_address const* _addr) noexcept
{
    auto& env = static_cast<ExtVMFace&>(*_context);
    return toBvmC(env.balance(fromBvmC(*_addr)));
}

size_t getCodeSize(bvmc_context* _context, bvmc_address const* _addr)
{
    auto& env = static_cast<ExtVMFace&>(*_context);
    return env.codeSizeAt(fromBvmC(*_addr));
}

bvmc_bytes32 getCodeHash(bvmc_context* _context, bvmc_address const* _addr)
{
    auto& env = static_cast<ExtVMFace&>(*_context);
    return toBvmC(env.codeHashAt(fromBvmC(*_addr)));
}

size_t copyCode(bvmc_context* _context, bvmc_address const* _addr, size_t _codeOffset,
    byte* _bufferData, size_t _bufferSize)
{
    auto& env = static_cast<ExtVMFace&>(*_context);
    Address addr = fromBvmC(*_addr);
    bytes const& code = env.codeAt(addr);

    // Handle "big offset" edge case.
    if (_codeOffset >= code.size())
        return 0;

    size_t maxToCopy = code.size() - _codeOffset;
    size_t numToCopy = std::min(maxToCopy, _bufferSize);
    std::copy_n(&code[_codeOffset], numToCopy, _bufferData);
    return numToCopy;
}

void selfdestruct(
    bvmc_context* _context,
    bvmc_address const* _addr,
    bvmc_address const* _beneficiary
) noexcept
{
    (void) _addr;
    auto& env = static_cast<ExtVMFace&>(*_context);
    assert(fromBvmC(*_addr) == env.myAddress);
    env.suicide(fromBvmC(*_beneficiary));
}


void log(
    bvmc_context* _context,
    bvmc_address const* _addr,
    uint8_t const* _data,
    size_t _dataSize,
    bvmc_uint256be const _topics[],
    size_t _numTopics
) noexcept
{
    (void) _addr;
    auto& env = static_cast<ExtVMFace&>(*_context);
    assert(fromBvmC(*_addr) == env.myAddress);
    h256 const* pTopics = reinterpret_cast<h256 const*>(_topics);
    env.log(h256s{pTopics, pTopics + _numTopics},
            bytesConstRef{_data, _dataSize});
}

bvmc_tx_context getTxContext(bvmc_context* _context) noexcept
{
    auto& env = static_cast<ExtVMFace&>(*_context);
    bvmc_tx_context result = {};
    result.tx_gas_price = toBvmC(env.gasPrice);
    result.tx_origin = toBvmC(env.origin);
    result.block_coinbase = toBvmC(env.envInfo().author());
    result.block_number = static_cast<int64_t>(env.envInfo().number());
    result.block_timestamp = static_cast<int64_t>(env.envInfo().timestamp());
    result.block_gas_limit = static_cast<int64_t>(env.envInfo().gasLimit());
    result.block_difficulty = toBvmC(env.envInfo().difficulty());
    return result;
}

bvmc_bytes32 getBlockHash(bvmc_context* _envPtr, int64_t _number)
{
    auto& env = static_cast<ExtVMFace&>(*_envPtr);
    return toBvmC(env.blockHash(_number));
}

bvmc_result create(ExtVMFace& _env, bvmc_message const* _msg) noexcept
{
    u256 gas = _msg->gas;
    u256 value = fromBvmC(_msg->value);
    bytesConstRef init = {_msg->input_data, _msg->input_size};
    u256 salt = fromBvmC(_msg->create2_salt);
    Instruction opcode = _msg->kind == BVMC_CREATE ? Instruction::CREATE : Instruction::CREATE2;

    // ExtVM::create takes the sender address from .myAddress.
    assert(fromBvmC(_msg->sender) == _env.myAddress);

    CreateResult result = _env.create(value, gas, init, opcode, salt, {});
    bvmc_result bvmcResult = {};
    bvmcResult.status_code = result.status;
    bvmcResult.gas_left = static_cast<int64_t>(gas);

    if (result.status == BVMC_SUCCESS)
        bvmcResult.create_address = toBvmC(result.address);
    else
    {
        // Pass the output to the BRC without a copy. The BRC will delete it
        // when finished with it.

        // First assign reference. References are not invalidated when vector
        // of bytes is moved. See `.takeBytes()` below.
        bvmcResult.output_data = result.output.data();
        bvmcResult.output_size = result.output.size();

        // Place a new vector of bytes containing output in result's reserved memory.
        auto* data = bvmc_get_optional_storage(&bvmcResult);
        static_assert(sizeof(bytes) <= sizeof(*data), "Vector is too big");
        new(data) bytes(result.output.takeBytes());
        // Set the destructor to delete the vector.
        bvmcResult.release = [](bvmc_result const* _result)
        {
            auto* data = bvmc_get_const_optional_storage(_result);
            auto& output = reinterpret_cast<bytes const&>(*data);
            // Explicitly call vector's destructor to release its data.
            // This is normal pattern when placement new operator is used.
            output.~bytes();
        };
    }
    return bvmcResult;
}

bvmc_result call(bvmc_context* _context, bvmc_message const* _msg) noexcept
{
    assert(_msg->gas >= 0 && "Invalid gas value");
    auto& env = static_cast<ExtVMFace&>(*_context);

    // Handle CREATE separately.
    if (_msg->kind == BVMC_CREATE || _msg->kind == BVMC_CREATE2)
        return create(env, _msg);

    CallParameters params;
    params.gas = _msg->gas;
    params.apparentValue = fromBvmC(_msg->value);
    params.valueTransfer =
        _msg->kind == BVMC_DELEGATECALL ? 0 : params.apparentValue;
    params.senderAddress = fromBvmC(_msg->sender);
    params.codeAddress = fromBvmC(_msg->destination);
    params.receiveAddress =
        _msg->kind == BVMC_CALL ? params.codeAddress : env.myAddress;
    params.data = {_msg->input_data, _msg->input_size};
    params.staticCall = (_msg->flags & BVMC_STATIC) != 0;
    params.onOp = {};

    CallResult result = env.call(params);
    bvmc_result bvmcResult = {};
    bvmcResult.status_code = result.status;
    bvmcResult.gas_left = static_cast<int64_t>(params.gas);

    // Pass the output to the BRC without a copy. The BRC will delete it
    // when finished with it.

    // First assign reference. References are not invalidated when vector
    // of bytes is moved. See `.takeBytes()` below.
    bvmcResult.output_data = result.output.data();
    bvmcResult.output_size = result.output.size();

    // Place a new vector of bytes containing output in result's reserved memory.
    auto* data = bvmc_get_optional_storage(&bvmcResult);
    static_assert(sizeof(bytes) <= sizeof(*data), "Vector is too big");
    new(data) bytes(result.output.takeBytes());
    // Set the destructor to delete the vector.
    bvmcResult.release = [](bvmc_result const* _result)
    {
        auto* data = bvmc_get_const_optional_storage(_result);
        auto& output = reinterpret_cast<bytes const&>(*data);
        // Explicitly call vector's destructor to release its data.
        // This is normal pattern when placement new operator is used.
        output.~bytes();
    };
    return bvmcResult;
}

bvmc_host_interface const hostInterface = {
    accountExists,
    getStorage,
    setStorage,
    getBalance,
    getCodeSize,
    getCodeHash,
    copyCode,
    selfdestruct,
    brc::call,
    getTxContext,
    getBlockHash,
    brc::log,
};
}

ExtVMFace::ExtVMFace(EnvInfo const& _envInfo, Address _myAddress, Address _caller, Address _origin,
    u256 _value, u256 _gasPrice, bytesConstRef _data, bytes _code, h256 const& _codeHash,
    unsigned _depth, bool _isCreate, bool _staticCall)
  : bvmc_context{&hostInterface},
    m_envInfo(_envInfo),
    myAddress(_myAddress),
    caller(_caller),
    origin(_origin),
    value(_value),
    gasPrice(_gasPrice),
    data(_data),
    code(std::move(_code)),
    codeHash(_codeHash),
    depth(_depth),
    isCreate(_isCreate),
    staticCall(_staticCall)
{}

}
}
