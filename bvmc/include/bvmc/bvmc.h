
#ifndef BVMC_H
#define BVMC_H

#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 6)
/**
 * Portable declaration of "deprecated" attribute.
 *
 * Available for clang and GCC 6+ compilers. The older GCC compilers know
 * this attribute, but it cannot be applied to enum elements.
 */
#define BVMC_DEPRECATED __attribute__((deprecated))
#else
#define BVMC_DEPRECATED
#endif


#include <stdbool.h> /* Definition of bool, true and false. */
#include <stddef.h>  /* Definition of size_t. */
#include <stdint.h>  /* Definition of int64_t, uint64_t. */

#if __cplusplus
extern "C" {
#endif

/* BEGIN Python CFFI declarations */

enum
{
    BVMC_ABI_VERSION = 6
};


/**
 * The fixed size array of 32 bytes.
 *
 * 32 bytes of data capable of storing e.g. 256-bit hashes.
 */
typedef struct bvmc_bytes32
{
    /** The 32 bytes. */
    uint8_t bytes[32];
} bvmc_bytes32;

/**
 * The alias for bvmc_bytes32 to represent a big-endian 256-bit integer.
 */
typedef struct bvmc_bytes32 bvmc_uint256be;

/** Big-endian 160-bit hash suitable for keeping an eum address. */
typedef struct bvmc_address
{
    /** The 20 bytes of the hash. */
    uint8_t bytes[20];
} bvmc_address;

/** The kind of call-like instruction. */
enum bvmc_call_kind
{
    BVMC_CALL = 0,         /**< Request CALL. */
    BVMC_DELEGATECALL = 1, /**< Request DELEGATECALL. Valid since Homestead.
                                The value param ignored. */
    BVMC_CALLCODE = 2,     /**< Request CALLCODE. */
    BVMC_CREATE = 3,       /**< Request CREATE. */
    BVMC_CREATE2 = 4       /**< Request CREATE2. Valid since Constantinople.*/
};

/** The flags for ::bvmc_message. */
enum bvmc_flags
{
    BVMC_STATIC = 1 /**< Static call mode. */
};

/**
 * The message describing an BVM call,
 * including a zero-depth calls from a transaction origin.
 */
struct bvmc_message
{
    /** The kind of the call. For zero-depth calls ::BVMC_CALL SHOULD be used. */
    enum bvmc_call_kind kind;

    /**
     * Additional flags modifying the call execution behavior.
     * In the current version the only valid values are ::BVMC_STATIC or 0.
     */
    uint32_t flags;

    /** The call depth. */
    int32_t depth;

    /** The amount of gas for message execution. */
    int64_t gas;

    /** The destination of the message. */
    bvmc_address destination;

    /** The sender of the message. */
    bvmc_address sender;

    /**
     * The message input data.
     *
     * This MAY be NULL.
     */
    const uint8_t* input_data;

    /**
     * The size of the message input data.
     *
     * If input_data is NULL this MUST be 0.
     */
    size_t input_size;

    /**
     * The amount of  transferred with the message.
     */
    bvmc_uint256be value;

    /**
     * The optional value used in new contract address construction.
     *
     * Ignored unless kind is BVMC_CREATE2.
     */
    bvmc_bytes32 create2_salt;
};


/** The transaction and block data for execution. */
struct bvmc_tx_context
{
    bvmc_uint256be tx_gas_price;     /**< The transaction gas price. */
    bvmc_address tx_origin;          /**< The transaction origin account. */
    bvmc_address block_coinbase;     /**< The miner of the block. */
    int64_t block_number;            /**< The block number. */
    int64_t block_timestamp;         /**< The block timestamp. */
    int64_t block_gas_limit;         /**< The block gas limit. */
    bvmc_uint256be block_difficulty; /**< The block difficulty. */
};

struct bvmc_context;

/**
 * Get transaction context callback function.
 *
 *  This callback function is used by an BVM to retrieve the transaction and
 *  block context.
 *
 *  @param      context  The pointer to the Host execution context.
 *  @return              The transaction context.
 */
typedef struct bvmc_tx_context (*bvmc_get_tx_context_fn)(struct bvmc_context* context);

/**
 * Get block hash callback function.
 *
 * This callback function is used by a VM to query the hash of the header of the given block.
 * If the information about the requested block is not available, then this is signalled by
 * returning null bytes.
 *
 * @param context  The pointer to the Host execution context.
 * @param number   The block number.
 * @return         The block hash or null bytes
 *                 if the information about the block is not available.
 */
typedef bvmc_bytes32 (*bvmc_get_block_hash_fn)(struct bvmc_context* context, int64_t number);

/**
 * The execution status code.
 *
 * Successful execution is represented by ::BVMC_SUCCESS having value 0.
 *
 * Positive values represent failures defined by VM specifications with generic
 * ::BVMC_FAILURE code of value 1.
 *
 * Status codes with negative values represent VM internal errors
 * not provided by BVM specifications. These errors MUST not be passed back
 * to the caller. They MAY be handled by the Client in predefined manner
 * (see e.g. ::BVMC_REJECTED), otherwise internal errors are not recoverable.
 * The generic representant of errors is ::BVMC_INTERNAL_ERROR but
 * an BVM implementation MAY return negative status codes that are not defined
 * in the BVMC documentation.
 *
 * @note
 * In case new status codes are needed, please create an issue or pull request
 * in the BVMC repository (ereum/bvmc).
 */
enum bvmc_status_code
{
    /** Execution finished with success. */
    BVMC_SUCCESS = 0,

    /** Generic execution failure. */
    BVMC_FAILURE = 1,

    /**
     * Execution terminated with REVERT opcode.
     *
     * In this case the amount of gas left MAY be non-zero and additional output
     * data MAY be provided in ::bvmc_result.
     */
    BVMC_REVERT = 2,

    /** The execution has run out of gas. */
    BVMC_OUT_OF_GAS = 3,

    /**
     * The designated INVALID instruction has been hit during execution.
     *
     * The EIP-141 (ereum/EIPs/blob/master/EIPS/eip-141.md)
     * defines the instruction 0xfe as INVALID instruction to indicate execution
     * abortion coming from high-level languages. This status code is reported
     * in case this INVALID instruction has been encountered.
     */
    BVMC_INVALID_INSTRUCTION = 4,

    /** An undefined instruction has been encountered. */
    BVMC_UNDEFINED_INSTRUCTION = 5,

    /**
     * The execution has attempted to put more items on the BVM stack
     * than the specified limit.
     */
    BVMC_STACK_OVERFLOW = 6,

    /** Execution of an opcode has required more items on the BVM stack. */
    BVMC_STACK_UNDERFLOW = 7,

    /** Execution has violated the jump destination restrictions. */
    BVMC_BAD_JUMP_DESTINATION = 8,

    /**
     * Tried to read outside memory bounds.
     *
     * An example is RETURNDATACOPY reading past the available buffer.
     */
    BVMC_INVALID_MEMORY_ACCESS = 9,

    /** Call depth has exceeded the limit (if any) */
    BVMC_CALL_DEPTH_EXCEEDED = 10,

    /** Tried to execute an operation which is restricted in static mode. */
    BVMC_STATIC_MODE_VIOLATION = 11,

    /**
     * A call to a precompiled or system contract has ended with a failure.
     *
     * An example: elliptic curve functions handed invalid EC points.
     */
    BVMC_PRECOMPILE_FAILURE = 12,

    /**
     * Contract validation has failed (e.g. due to BVM 1.5 jump validity,
     * Casper's purity checker or ewasm contract rules).
     */
    BVMC_CONTRACT_VALIDATION_FAILURE = 13,

    /**
     * An argument to a state accessing method has a value outside of the
     * accepted range of values.
     */
    BVMC_ARGUMENT_OUT_OF_RANGE = 14,

    /**
     * A WebAssembly `unreachable` instruction has been hit during execution.
     */
    BVMC_WASM_UNREACHABLE_INSTRUCTION = 15,

    /**
     * A WebAssembly trap has been hit during execution. This can be for many
     * reasons, including division by zero, validation errors, etc.
     */
    BVMC_WASM_TRAP = 16,

    /** BVM implementation generic internal error. */
    BVMC_INTERNAL_ERROR = -1,

    /**
     * The execution of the given code and/or message has been rejected
     * by the BVM implementation.
     *
     * This error SHOULD be used to signal that the BVM is not able to or
     * willing to execute the given code type or message.
     * If an BVM returns the ::BVMC_REJECTED status code,
     * the Client MAY try to execute it in other BVM implementation.
     * For example, the Client tries running a code in the BVM 1.5. If the
     * code is not supported there, the execution falls back to the BVM 1.0.
     */
    BVMC_REJECTED = -2
};

/* Forward declaration. */
struct bvmc_result;

/**
 * Releases resources assigned to an execution result.
 *
 * This function releases memory (and other resources, if any) assigned to the
 * specified execution result making the result object invalid.
 *
 * @param result  The execution result which resources are to be released. The
 *                result itself it not modified by this function, but becomes
 *                invalid and user MUST discard it as well.
 *                This MUST NOT be NULL.
 *
 * @note
 * The result is passed by pointer to avoid (shallow) copy of the ::bvmc_result
 * struct. Think of this as the best possible C language approximation to
 * passing objects by reference.
 */
typedef void (*bvmc_release_result_fn)(const struct bvmc_result* result);

/** The BVM code execution result. */
struct bvmc_result
{
    /** The execution status code. */
    enum bvmc_status_code status_code;

    /**
     * The amount of gas left after the execution.
     *
     *  If bvmc_result::code is not ::BVMC_SUCCESS nor ::BVMC_REVERT
     *  the value MUST be 0.
     */
    int64_t gas_left;

    /**
     * The reference to output data.
     *
     *  The output contains data coming from RETURN opcode (iff bvmc_result::code
     *  field is ::BVMC_SUCCESS) or from REVERT opcode.
     *
     *  The memory containing the output data is owned by BVM and has to be
     *  freed with bvmc_result::release().
     *
     *  This MAY be NULL.
     */
    const uint8_t* output_data;

    /**
     * The size of the output data.
     *
     *  If output_data is NULL this MUST be 0.
     */
    size_t output_size;

    /**
     * The pointer to a function releasing all resources associated with
     *  the result object.
     *
     *  This function pointer is optional (MAY be NULL) and MAY be set by
     *  the BVM implementation. If set it MUST be used by the user to
     *  release memory and other resources associated with the result object.
     *  After the result resources are released the result object
     *  MUST NOT be used any more.
     *
     *  The suggested code pattern for releasing BVM results:
     *  @code
     *  struct bvmc_result result = ...;
     *  if (result.release)
     *      result.release(&result);
     *  @endcode
     *
     *  @note
     *  It works similarly to C++ virtual destructor. Attaching the release
     *  function to the result itself allows BVM composition.
     */
    bvmc_release_result_fn release;

    /**
     * The address of the contract created by CREATE opcode.
     *
     *  This field has valid value only if the result describes successful
     *  CREATE (bvmc_result::status_code is ::BVMC_SUCCESS).
     */
    bvmc_address create_address;

    /**
     * Reserved data that MAY be used by a bvmc_result object creator.
     *
     *  This reserved 4 bytes tog with 20 bytes from create_address form
     *  24 bytes of memory called "optional data" within bvmc_result struct
     *  to be optionally used by the bvmc_result object creator.
     *
     *  @see bvmc_result_optional_data, bvmc_get_optional_data().
     *
     *  Also extends the size of the bvmc_result to 64 bytes (full cache line).
     */
    uint8_t padding[4];
};


/**
 * Check account existence callback function.
 *
 * This callback function is used by the VM to check if
 * there exists an account at given address.
 * @param context  The pointer to the Host execution context.
 * @param address  The address of the account the query is about.
 * @return         true if exists, false otherwise.
 */
typedef bool (*bvmc_account_exists_fn)(struct bvmc_context* context, const bvmc_address* address);

/**
 * Get storage callback function.
 *
 * This callback function is used by a VM to query the given account storage entry.
 *
 * @param context  The Host execution context.
 * @param address  The address of the account.
 * @param key      The index of the account's storage entry.
 * @return         The storage value at the given storage key or null bytes
 *                 if the account does not exist.
 */
typedef bvmc_bytes32 (*bvmc_get_storage_fn)(struct bvmc_context* context,
                                            const bvmc_address* address,
                                            const bvmc_bytes32* key);


/**
 * The effect of an attempt to modify a contract storage item.
 *
 * For the purpose of explaining the meaning of each element, the following
 * notation is used:
 * - 0 is zero value,
 * - X != 0 (X is any value other than 0),
 * - Y != X, Y != 0 (Y is any value other than X and 0),
 * - Z != Y (Z is any value other than Y),
 * - the "->" means the change from one value to another.
 */
enum bvmc_storage_status
{
    /**
     * The value of a storage item has been left unchanged: 0 -> 0 and X -> X.
     */
    BVMC_STORAGE_UNCHANGED = 0,

    /**
     * The value of a storage item has been modified: X -> Y.
     */
    BVMC_STORAGE_MODIFIED = 1,

    /**
     * A storage item has been modified after being modified before: X -> Y -> Z.
     */
    BVMC_STORAGE_MODIFIED_AGAIN = 2,

    /**
     * A new storage item has been added: 0 -> X.
     */
    BVMC_STORAGE_ADDED = 3,

    /**
     * A storage item has been deleted: X -> 0.
     */
    BVMC_STORAGE_DELETED = 4
};


/**
 * Set storage callback function.
 *
 * This callback function is used by a VM to update the given account storage entry.
 * The VM MUST make sure that the account exists. This requirement is only a formality because
 * VM implementations only modify storage of the account of the current execution context
 * (i.e. referenced by bvmc_message::destination).
 *
 * @param context  The pointer to the Host execution context.
 * @param address  The address of the account.
 * @param key      The index of the storage entry.
 * @param value    The value to be stored.
 * @return         The effect on the storage item.
 */
typedef enum bvmc_storage_status (*bvmc_set_storage_fn)(struct bvmc_context* context,
                                                        const bvmc_address* address,
                                                        const bvmc_bytes32* key,
                                                        const bvmc_bytes32* value);

/**
 * Get balance callback function.
 *
 * This callback function is used by a VM to query the balance of the given account.
 *
 * @param context  The pointer to the Host execution context.
 * @param address  The address of the account.
 * @return         The balance of the given account or 0 if the account does not exist.
 */
typedef bvmc_uint256be (*bvmc_get_balance_fn)(struct bvmc_context* context,
                                              const bvmc_address* address);

/**
 * Get code size callback function.
 *
 * This callback function is used by a VM to get the size of the code stored
 * in the account at the given address.
 *
 * @param context  The pointer to the Host execution context.
 * @param address  The address of the account.
 * @return         The size of the code in the account or 0 if the account does not exist.
 */
typedef size_t (*bvmc_get_code_size_fn)(struct bvmc_context* context, const bvmc_address* address);

/**
 * Get code size callback function.
 *
 * This callback function is used by a VM to get the keccak256 hash of the code stored
 * in the account at the given address. For existing accounts not having a code, this
 * function returns keccak256 hash of empty data.
 *
 * @param context  The pointer to the Host execution context.
 * @param address  The address of the account.
 * @return         The hash of the code in the account or null bytes if the account does not exist.
 */
typedef bvmc_bytes32 (*bvmc_get_code_hash_fn)(struct bvmc_context* context,
                                              const bvmc_address* address);

/**
 * Copy code callback function.
 *
 *  This callback function is used by an BVM to request a copy of the code
 *  of the given account to the memory buffer provided by the BVM.
 *  The Client MUST copy the requested code, starting with the given offset,
 *  to the provided memory buffer up to the size of the buffer or the size of
 *  the code, whichever is smaller.
 *
 *  @param context      The pointer to the Client execution context.
 *                           @see ::bvmc_context.
 *  @param address      The address of the account.
 *  @param code_offset  The offset of the code to copy.
 *  @param buffer_data  The pointer to the memory buffer allocated by the BVM
 *                      to store a copy of the requested code.
 *  @param buffer_size  The size of the memory buffer.
 *  @return             The number of bytes copied to the buffer by the Client.
 */
typedef size_t (*bvmc_copy_code_fn)(struct bvmc_context* context,
                                    const bvmc_address* address,
                                    size_t code_offset,
                                    uint8_t* buffer_data,
                                    size_t buffer_size);

/**
 * Selfdestruct callback function.
 *
 *  This callback function is used by an BVM to SELFDESTRUCT given contract.
 *  The execution of the contract will not be stopped, that is up to the BVM.
 *
 *  @param context      The pointer to the Host execution context.
 *                      @see ::bvmc_context.
 *  @param address      The address of the contract to be selfdestructed.
 *  @param beneficiary  The address where the remaining  is going to be
 *                      transferred.
 */
typedef void (*bvmc_selfdestruct_fn)(struct bvmc_context* context,
                                     const bvmc_address* address,
                                     const bvmc_address* beneficiary);

/**
 * Log callback function.
 *
 *  This callback function is used by an BVM to inform about a LOG that happened
 *  during an BVM bytecode execution.
 *  @param context       The pointer to the Host execution context.
 *                       @see ::bvmc_context.
 *  @param address       The address of the contract that generated the log.
 *  @param data          The pointer to unindexed data attached to the log.
 *  @param data_size     The length of the data.
 *  @param topics        The pointer to the array of topics attached to the log.
 *  @param topics_count  The number of the topics. Valid values are between
 *                       0 and 4 inclusively.
 */
typedef void (*bvmc_emit_log_fn)(struct bvmc_context* context,
                                 const bvmc_address* address,
                                 const uint8_t* data,
                                 size_t data_size,
                                 const bvmc_bytes32 topics[],
                                 size_t topics_count);

/**
 * Pointer to the callback function supporting BVM calls.
 *
 * @param  context The pointer to the Host execution context.
 * @param  msg     The call parameters.
 * @return         The result of the call.
 */
typedef struct bvmc_result (*bvmc_call_fn)(struct bvmc_context* context,
                                           const struct bvmc_message* msg);

/**
 * The Host interface.
 *
 * The set of all callback functions expected by VM instances. This is C
 * realisation of vtable for OOP interface (only virtual methods, no data).
 * Host implementations SHOULD create constant singletons of this (similarly
 * to vtables) to lower the maintenance and memory management cost.
 */
struct bvmc_host_interface
{
    /** Check account existence callback function. */
    bvmc_account_exists_fn account_exists;

    /** Get storage callback function. */
    bvmc_get_storage_fn get_storage;

    /** Set storage callback function. */
    bvmc_set_storage_fn set_storage;

    /** Get balance callback function. */
    bvmc_get_balance_fn get_balance;

    /** Get code size callback function. */
    bvmc_get_code_size_fn get_code_size;

    /** Get code hash callback function. */
    bvmc_get_code_hash_fn get_code_hash;

    /** Copy code callback function. */
    bvmc_copy_code_fn copy_code;

    /** Selfdestruct callback function. */
    bvmc_selfdestruct_fn selfdestruct;

    /** Call callback function. */
    bvmc_call_fn call;

    /** Get transaction context callback function. */
    bvmc_get_tx_context_fn get_tx_context;

    /** Get block hash callback function. */
    bvmc_get_block_hash_fn get_block_hash;

    /** Emit log callback function. */
    bvmc_emit_log_fn emit_log;
};


/**
 * Execution context managed by the Host.
 *
 *  The Host MUST pass the pointer to the execution context to
 *  ::bvmc_execute_fn. The BVM MUST pass the same pointer back to the Host in
 *  every callback function.
 *  The context MUST contain at least the function table defining the context
 *  callback interface.
 *  Optionally, The Host MAY include in the context additional data.
 */
struct bvmc_context
{
    /** The Host interface. */
    const struct bvmc_host_interface* host;
};


/* Forward declaration. */
struct bvmc_instance;

/**
 * Destroys the BVM instance.
 *
 *  @param bvm  The BVM instance to be destroyed.
 */
typedef void (*bvmc_destroy_fn)(struct bvmc_instance* bvm);

/**
 * Possible outcomes of bvmc_set_option.
 */
enum bvmc_set_option_result
{
    BVMC_SET_OPTION_SUCCESS = 0,
    BVMC_SET_OPTION_INVALID_NAME = 1,
    BVMC_SET_OPTION_INVALID_VALUE = 2
};

/**
 * Configures the BVM instance.
 *
 *  Allows modifying options of the BVM instance.
 *  Options:
 *  - code cache behavior: on, off, read-only, ...
 *  - optimizations,
 *
 *  @param bvm    The BVM instance to be configured.
 *  @param name   The option name. NULL-terminated string. Cannot be NULL.
 *  @param value  The new option value. NULL-terminated string. Cannot be NULL.
 *  @return       The outcome of the operation.
 */
typedef enum bvmc_set_option_result (*bvmc_set_option_fn)(struct bvmc_instance* bvm,
                                                          char const* name,
                                                          char const* value);


/**
 * BVM revision.
 *
 * The revision of the BVM specification based on the eum
 * upgrade / hard fork codenames.
 */
enum bvmc_revision
{
    /**
     * The Frontier revision.
     *
     * The one eum launched with.
     */
    BVMC_FRONTIER = 0,

    /**
     * The Homestead revision.
     *
     */
    BVMC_HOMESTEAD = 1,

    /**
     * The Tangerine Whistle revision.
     *
     */
    BVMC_TANGERINE_WHISTLE = 2,

    /**
     * The Spurious Dragon revision.
     *
     */
    BVMC_SPURIOUS_DRAGON = 3,

    /**
     * The Byzantium revision.
     *
     */
    BVMC_BYZANTIUM = 4,

    /**
     * The Constantinople revision.
     *
     */
    BVMC_CONSTANTINOPLE = 5,

    /**
     * Reserved for the post-Constantinople upgrade. The name is likely to
     * be changed, but the assigned number should stay.
     *
     * The spec draft: ereum/EIPs/pull/1716.
     */
    BVMC_CONSTANTINOPLE2 = 6,

    /**
     * The Istanbul revision.
     *
     * The spec draft: ereum.org/EIPS/eip-1679.
     */
    BVMC_ISTANBUL = 7,

    /** The maximum BVM revision supported. */
    BVMC_MAX_REVISION = BVMC_ISTANBUL,


    /**
     * The latests BVM revision supported.
     *
     * @deprecated Replaced with ::BVMC_MAX_REVISION.
     */
    BVMC_LATEST_REVISION BVMC_DEPRECATED = BVMC_MAX_REVISION
};


/**
 * Executes the given BVM bytecode using the input in the message
 *
 * This function MAY be invoked multiple times for a single BVM instance.
 *
 * @param instance   The BVM instance.
 * @param context    The pointer to the Client execution context to be passed
 *                   to the callback functions. @see ::bvmc_context.
 * @param rev        Requested BVM specification revision.
 * @param msg        Call parameters. @see ::bvmc_message.
 * @param code       Reference to the bytecode to be executed.
 * @param code_size  The length of the bytecode.
 * @return           All execution results.
 */
typedef struct bvmc_result (*bvmc_execute_fn)(struct bvmc_instance* instance,
                                              struct bvmc_context* context,
                                              enum bvmc_revision rev,
                                              const struct bvmc_message* msg,
                                              uint8_t const* code,
                                              size_t code_size);

/**
 * Possible capabilities of a VM.
 */
enum bvmc_capabilities
{
    BVMC_CAPABILITY_BVM1 = (1u << 0), /**< The VM is capable of executing BVM1 bytecode. */
    BVMC_CAPABILITY_EWASM = (1u << 1) /**< The VM is capable of execution ewasm bytecode. */
};

/**
 * Alias for unsigned integer representing a set of bit flags of BVMC capabilities.
 *
 * @see bvmc_capabilities
 */
typedef uint32_t bvmc_capabilities_flagset;

/**
 * Return the supported capabilities of the VM instance.
 *
 * This function MAY be invoked multiple times for a single VM instance,
 * and its value MAY be influenced by calls to bvmc_instance::set_option.
 *
 * @param instance  The BVM instance.
 * @return          The supported capabilities of the VM. @see bvmc_capabilities.
 */
typedef bvmc_capabilities_flagset (*bvmc_get_capabilities_fn)(struct bvmc_instance* instance);

/** The opaque type representing a Client-side tracer object. */
struct bvmc_tracer_context;

/**
 * The callback to trace instructions execution in an BVM.
 *
 * This function informs the Client what instruction has been executed in the BVM implementation
 * and what are the results of executing this particular instruction.
 * The message level information (like call depth, destination address, etc.) are not provided here.
 * This piece of information can be acquired by inspecting messages being sent to the BVM in
 * ::bvmc_execute_fn and the results of the messages execution.
 *
 * @param context                The pointer to the Client-side tracing context. This allows to
 *                               implement the tracer in OOP manner.
 * @param code_offset            The current instruction position in the code.
 * @param status_code            The status code of the instruction execution.
 * @param gas_left               The amount of the gas left after the instruction execution.
 * @param stack_num_items        The current BVM stack height after the instruction execution.
 * @param pushed_stack_item      The top BVM stack item pushed as the result of the instruction
 *                               execution. This value is null when the instruction does not push
 *                               anything to the stack.
 * @param memory_size            The size of the BVM memory after the instruction execution.
 * @param changed_memory_offset  The offset in number of bytes of the beginning of the memory area
 *                               modified as the result of the instruction execution.
 *                               The Client MAY use this information tog with
 *                               @p changed_memory_size and @p changed_memory to incrementally
 *                               update the copy of the full VM's memory.
 * @param changed_memory_size    The size of the memory area modified as the result of
 *                               the instruction execution.
 * @param changed_memory         The pointer to the memory area modified as the result of
 *                               the instruction execution.
 *                               The Client MAY access the pointed memory area
 *                               (limited by the @p changed_memory_size) only during the current
 *                               execution of the bvmc_trace_callback().
 *                               The pointer MUST NOT be stored by the Client.
 *                               The Client MUST NOT assume that
 *                               `changed_memory - changed_memory_offset` is a valid base pointer
 *                               of the VM memory.
 */
typedef void (*bvmc_trace_callback)(struct bvmc_tracer_context* context,
                                    size_t code_offset,
                                    enum bvmc_status_code status_code,
                                    int64_t gas_left,
                                    size_t stack_num_items,
                                    const bvmc_uint256be* pushed_stack_item,
                                    size_t memory_size,
                                    size_t changed_memory_offset,
                                    size_t changed_memory_size,
                                    const uint8_t* changed_memory);

/**
 * Sets the BVM instruction tracer.
 *
 * When the tracer is set in the BVM instance, the BVM SHOULD call back the tracer with information
 * about instructions execution in the BVM.
 * @see ::bvmc_trace_callback.
 *
 * This will overwrite the previous settings (the callback and the context).
 *
 * @param instance    The BVM instance.
 * @param callback    The tracer callback function. This argument MAY be NULL to disable previously
 *                    set tracer.
 * @param context     The Client-side tracer context. This argument MAY be NULL in case the tracer
 *                    does not require any context. This argument MUST be NULL if the callback
 *                    argument is NULL.
 */
typedef void (*bvmc_set_tracer_fn)(struct bvmc_instance* instance,
                                   bvmc_trace_callback callback,
                                   struct bvmc_tracer_context* context);


/**
 * The BVM instance.
 *
 *  Defines the base struct of the BVM implementation.
 */
struct bvmc_instance
{
    /**
     *  BVMC ABI version implemented by the BVM instance.
     *
     *  Used to detect ABI incompatibilities. The BVMC ABI version
     *  represented by this file is in ::BVMC_ABI_VERSION.
     */
    const int abi_version;

    /**
     * The name of the BVMC VM implementation.
     *
     *  It MUST be a NULL-terminated not empty string.
     */
    const char* name;

    /**
     * The version of the BVMC VM implementation, e.g. "1.2.3b4".
     *
     *  It MUST be a NULL-terminated not empty string.
     */
    const char* version;

    /** Pointer to function destroying the BVM instance. */
    bvmc_destroy_fn destroy;

    /** Pointer to function executing a code by the BVM instance. */
    bvmc_execute_fn execute;

    /**
     * Pointer to function returning capabilities supported by the VM instance.
     *
     * The value returned might change when different options are requested via set_option.
     *
     * A Client SHOULD only rely on the value returned here if it has queried it after
     * it has called set_option.
     */
    bvmc_get_capabilities_fn get_capabilities;

    /**
     * Optional pointer to function setting the BVM instruction tracer.
     *
     * If the BVM does not support this feature the pointer can be NULL.
     */
    bvmc_set_tracer_fn set_tracer;

    /**
     * Optional pointer to function modifying VM's options.
     *
     *  If the VM does not support this feature the pointer can be NULL.
     */
    bvmc_set_option_fn set_option;
};

/* END Python CFFI declarations */

#if BVMC_DOCUMENTATION
/**
 * Example of a function creating an instance of an example BVM implementation.
 *
 * Each BVM implementation MUST provide a function returning an BVM instance.
 * The function SHOULD be named `bvmc_create_<vm-name>(void)`. If the VM name contains hyphens
 * replaces them with underscores in the function names.
 *
 * @par Binaries naming convention
 * For VMs distributed as shared libraries, the name of the library SHOULD match the VM name.
 * The convetional library filename prefixes and extensions SHOULD be ignored by the Client.
 * For example, the shared library with the "beta-interpreter" implementation may be named
 * `libbeta-interpreter.so`.
 *
 * @return  BVM instance or NULL indicating instance creation failure.
 */
struct bvmc_instance* bvmc_create_example_vm(void);
#endif

#if __cplusplus
}
#endif

#endif
/** @} */
