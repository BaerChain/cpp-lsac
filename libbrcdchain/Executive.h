#pragma once

#include "BRCTranscation.h"
#include "DposVote.h"
#include "Transaction.h"
#include <libdevcore/Log.h>
#include <libbrccore/Common.h>
#include <libbvm/VMFace.h>

#include <json/json.h>
#include <functional>

namespace Json
{
class Value;
}

namespace dev
{
class OverlayDB;

namespace brc
{
class State;
class Block;
class BlockChain;
class ExtVM;
class SealEngineFace;
struct Manifest;

class StandardTrace
{
public:
    struct DebugOptions
    {
        bool disableStorage = false;
        bool disableMemory = false;
        bool disableStack = false;
        bool fullStorage = false;
    };

    StandardTrace();
    void operator()(uint64_t _steps, uint64_t _PC, Instruction _inst, bigint _newMemSize,
        bigint _gasCost, bigint _gas, VMFace const* _vm, ExtVMFace const* _extVM);

    void setShowMnemonics() { m_showMnemonics = true; }
    void setOptions(DebugOptions _options) { m_options = _options; }

    Json::Value jsonValue() const { return m_trace; }
    std::string styledJson() const;
    std::string multilineTrace() const;

    OnOpFunc onOp()
    {
        return [=](uint64_t _steps, uint64_t _PC, Instruction _inst, bigint _newMemSize,
                   bigint _gasCost, bigint _gas, VMFace const* _vm, ExtVMFace const* _extVM) {
            (*this)(_steps, _PC, _inst, _newMemSize, _gasCost, _gas, _vm, _extVM);
        };
    }

private:
    bool m_showMnemonics = false;
    std::vector<Instruction> m_lastInst;
    Json::Value m_trace;
    DebugOptions m_options;
};

/**
 * @brief Message-call/contract-creation executor; useful for executing transactions.
 *
 * Two ways of using this class - either as a transaction executive or a CALL/CREATE executive.
 *
 * In the first use, after construction, begin with initialize(), then execute() and end with
 * finalize(). Call go() after execute() only if it returns false.
 *
 * In the second use, after construction, begin with call() or create() and end with
 * accrueSubState(). Call go() after call()/create() only if it returns false.
 *
 * Example:
 * @code
 * Executive e(state, blockchain, 0);
 * e.initialize(transaction);
 * if (!e.execute())
 *    e.go();
 * e.finalize();
 * @endcode
 */
class Executive
{
public:
    /// Simple constructor; executive will operate on given state, with the given environment info.
    Executive(
        State& _s, EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, unsigned _level = 0)
      : m_vote(_s),
//        m_exdb(std::move(_s.exdb())),
        m_brctranscation(_s),
        m_s(_s),
        m_envInfo(_envInfo),
        m_depth(_level),
        m_sealEngine(_sealEngine)
    {}

    /** Easiest constructor.
     * Creates executive to operate on the state of end of the given block, populating environment
     * info from given Block and the LastHashes portion from the BlockChain.
     */
    Executive(Block& _s, BlockChain const& _bc, unsigned _level = 0);

    /** LastHashes-split constructor.
     * Creates executive to operate on the state of end of the given block, populating environment
     * info accordingly, with last hashes given explicitly.
     */
    Executive(Block& _s, LastBlockHashesFace const& _lh, unsigned _level = 0);

    /** Previous-state constructor.
     * Creates executive to operate on the state of a particular transaction in the given block,
     * populating environment info from the given Block and the LastHashes portion from the
     * BlockChain. State is assigned the resultant value, but otherwise unused.
     */
    Executive(State& io_s, Block const& _block, unsigned _txIndex, BlockChain const& _bc,
        unsigned _level = 0);

    Executive(Executive const&) = delete;
    void operator=(Executive) = delete;

    /// Initializes the executive for evaluating a transaction. You must call finalize() at some
    /// point following this.
    void initialize(bytesConstRef _transaction)
    {
        initialize(Transaction(_transaction, CheckTransaction::None));
    }
    void initialize(Transaction const& _transaction);
    /// Finalise a transaction previously set up with initialize().
    /// @warning Only valid after initialize() and execute(), and possibly go().
    /// @returns true if the outermost execution halted normally, false if exceptionally halted.
    bool finalize();
    /// Begins execution of a transaction. You must call finalize() following this.
    /// @returns true if the transaction is done, false if go() must be called.
    bool execute();
    /// @returns the transaction from initialize().
    /// @warning Only valid after initialize().
    Transaction const& t() const { return m_t; }
    /// @returns the log entries created by this operation.
    /// @warning Only valid after finalise().
    LogEntries const& logs() const { return m_logs; }
    /// @returns total gas used in the transaction/operation.
    /// @warning Only valid after finalise().
    u256 gasUsed() const;

    owning_bytes_ref takeOutput() { return std::move(m_output); }

    /// Set up the executive for evaluating a bare CREATE (contract-creation) operation.
    /// @returns false iff go() must be called (and thus a VM execution in required).
    bool create(Address const& _txSender, u256 const& _endowment, u256 const& _gasPrice,
        u256 const& _gas, bytesConstRef _code, Address const& _originAddress);
    /// @returns false iff go() must be called (and thus a VM execution in required).
    bool createOpcode(Address const& _sender, u256 const& _endowment, u256 const& _gasPrice,
        u256 const& _gas, bytesConstRef _code, Address const& _originAddress);
    /// @returns false iff go() must be called (and thus a VM execution in required).
    bool create2Opcode(Address const& _sender, u256 const& _endowment, u256 const& _gasPrice,
        u256 const& _gas, bytesConstRef _code, Address const& _originAddress, u256 const& _salt);
    /// Set up the executive for evaluating a bare CALL (message call) operation.
    /// @returns false iff go() must be called (and thus a VM execution in required).
    bool call(Address const& _receiveAddress, Address const& _txSender, u256 const& _txValue,
        u256 const& _gasPrice, bytesConstRef _txData, u256 const& _gas);
    bool call(CallParameters const& _cp, u256 const& _gasPrice, Address const& _origin);
    /// Finalise an operation through accruing the substate into the parent context.
    void accrueSubState(SubState& _parentContext);

    /// Executes (or continues execution of) the VM.
    /// @returns false iff go() must be called again to finish the transaction.
    bool go(OnOpFunc const& _onOp = OnOpFunc());

    /// Operation function for providing a simple trace of the VM execution.
    OnOpFunc simpleTrace();

    /// @returns gas remaining after the transaction/operation. Valid after the transaction has been
    /// executed.
    u256 gas() const { return m_gas; }

    /// @returns the new address for the created contract in the CREATE operation.
    Address newAddress() const { return m_newAddress; }

    /// @returns The exception that has happened during the execution if any.
    TransactionException getException() const noexcept { return m_excepted; }

    /// Collect execution results in the result storage provided.
    void setResultRecipient(ExecutionResult& _res) { m_res = &_res; }

    /// Revert all changes made to the state by this execution.
    void revert();

private:
	const u256 c_min_price = 5;
    enum Method
    {
        Other,
        VoteStart,
        BuyVotes,
		SellVotes, 
		LoginCandidate,
		LogOutCandidate,
        Vote,
        CancelVote,
		VoteEnd,
		TranscationStart,
        BRCTransaction,
		AssetInjection,
        TranscationEnd,
        PendingOrderStart,
        SellPendingOrder,
		BuyPendingOrder,
        CancelPendingOrder,
		PendingOrderEnd
    };

    struct TransationParameters
    {
        Method m_method = Other;
        CallParameters m_callParameters;
        u256 m_PendingOrderPrice = 0;
        h256 m_pendingOrderHash = h256(0);
        ex::order_token_type m_pendingOrder_Token_Type;
        ex::order_buy_type m_pendingOrder_Buy_Type;
		bigint m_total_gas_cost;
        TransationParameters(Method _type,
                CallParameters _c,
                u256 _pendingOrderPrice = 0,
            h256 _pendingOrderHash = h256(0),
            ex::order_token_type _pendingOrder_Token_Type = ex::order_token_type::BRC,
            ex::order_buy_type _pendingOrder_Buy_Type = ex::order_buy_type::all_price,
            bigint _total_gas_cost = 0)
          : m_method(_type),
            m_callParameters(_c),
            m_PendingOrderPrice(_pendingOrderPrice),
            m_pendingOrderHash(_pendingOrderHash),
			m_pendingOrder_Token_Type(_pendingOrder_Token_Type),
			m_pendingOrder_Buy_Type(_pendingOrder_Buy_Type),
            m_total_gas_cost(_total_gas_cost)
        {}
    };

    /// @returns false iff go() must be called (and thus a VM execution in required).
    bool executeCreate(Address const& _txSender, u256 const& _endowment, u256 const& _gasPrice,
        u256 const& _gas, bytesConstRef _code, Address const& _originAddress);
    void setCallParameters(Address const& _senderAddress, Address const& _codeAddress,
        Address const& _receiveAddress, u256 _valueTransfer, u256 _apparentValue, u256 _gas,
        bytesConstRef const& _data, OnOpFunc _onOpFunc);
    DposVote m_vote;  // dpos for vote class
    BRCTranscation m_brctranscation;
	ex::exchange_plugin m_exdb;

    State& m_s;  ///< The state to which this operation/transaction is applied.
    // TODO: consider changign to EnvInfo const& to avoid LastHashes copy at every CALL/CREATE
    EnvInfo m_envInfo;             ///< Information on the runtime environment.
    std::shared_ptr<ExtVM> m_ext;  ///< The VM externality object for the VM execution or null if no
                                   ///< VM is required. shared_ptr used only to allow ExtVM forward
                                   ///< reference. This field does *NOT* survive this object.
    owning_bytes_ref m_output;     ///< Execution output.
    ExecutionResult* m_res = nullptr;  ///< Optional storage for execution results.

    unsigned m_depth = 0;  ///< The context's call-depth.
    TransactionException m_excepted =
        TransactionException::None;  ///< Details if the VM's execution resulted in an exception.
    int64_t m_baseGasRequired;  ///< The base amount of gas requried for executing this transaction.
    u256 m_gas = 0;  ///< The gas for BRC code execution. Initial amount before go() execution,
                     ///< final amount after go() execution.

    Transaction m_t;    ///< The original transaction. Set by setup().
    LogEntries m_logs;  ///< The log entries created by this transaction. Set by finalize().

    u256 m_gasCost;
	u256 m_totalGas = 0;
	u256 m_addCostValue = 0;
	u256 m_needRefundGas = 0;
    u256 m_value = 0;
    u256 m_ballots = 0;
    bool m_pendingorderStatus = false;
    Method m_method = Other;
    CallParameters m_callParameters;  // transation callParameters
    SealEngineFace const& m_sealEngine;

    std::vector<TransationParameters> m_callParameters_v;  //  batchTransation callParameters

    bool m_isCreation = false;
    Address m_newAddress;
    size_t m_savepoint = 0;

    Logger m_execLogger{createLogger(VerbosityDebug, "exec")};
    Logger m_detailsLogger{createLogger(VerbosityTrace, "exec")};
    Logger m_vmTraceLogger{createLogger(VerbosityTrace, "vmtrace")};
};

}  // namespace brc
}  // namespace dev
