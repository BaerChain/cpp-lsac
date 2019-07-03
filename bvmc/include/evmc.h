#ifndef BVMC_H
#define BVMC_H

#include <stdint.h>    // Definition of int64_t, uint64_t.
#include <stddef.h>    // Definition of size_t.

#if __cplusplus
extern "C" {
#endif

// BEGIN Python CFFI declarations

enum
{
    /// The BVMC ABI version number of the interface declared in this file.
    BVMC_ABI_VERSION = 1
};

/// Big-endian 256-bit integer.
///
/// 32 bytes of data representing big-endian 256-bit integer. I.e. bytes[0] is
/// the most significant byte, bytes[31] is the least significant byte.
/// This type is used to transfer to/from the VM values interpreted by the user
/// as both 256-bit integers and 256-bit hashes.
struct bvmc_uint256be {
    /// The 32 bytes of the big-endian integer or hash.
    uint8_t bytes[32];
};

/// Big-endian 160-bit hash suitable for keeping an eum address.
struct bvmc_address {
    /// The 20 bytes of the hash.
    uint8_t bytes[20];
};

/// The kind of call-like instruction.
enum bvmc_call_kind {
    BVMC_CALL = 0,         ///< Request CALL.
    BVMC_DELEGATECALL = 1, ///< Request DELEGATECALL. The value param ignored.
    BVMC_CALLCODE = 2,     ///< Request CALLCODE.
    BVMC_CREATE = 3,       ///< Request CREATE. Semantic of some params changes.
};

/// The flags for ::bvmc_message.
enum bvmc_flags {
    BVMC_STATIC = 1        ///< Static call mode.
};

/// The message describing an BVM call,
/// including a zero-depth calls from a transaction origin.
struct bvmc_message {
    struct bvmc_address destination;  ///< The destination of the message.
    struct bvmc_address sender;       ///< The sender of the message.

    /// The amount of  transferred with the message.
    struct bvmc_uint256be value;

    /// The message input data.
    ///
    /// This MAY be NULL.
    const uint8_t* input_data;

    /// The size of the message input data.
    ///
    /// If input_data is NULL this MUST be 0.
    size_t input_size;

    /// The optional hash of the code of the destination account.
    /// The null hash MUST be used when not specified.
    struct bvmc_uint256be code_hash;

    int64_t gas;                 ///< The amount of gas for message execution.
    int32_t depth;               ///< The call depth.

    /// The kind of the call. For zero-depth calls ::BVMC_CALL SHOULD be used.
    enum bvmc_call_kind kind;

    /// Additional flags modifying the call execution behavior.
    /// In the current version the only valid values are ::BVMC_STATIC or 0.
    uint32_t flags;
};


/// The transaction and block data for execution.
struct bvmc_tx_context {
    struct bvmc_uint256be tx_gas_price;      ///< The transaction gas price.
    struct bvmc_address tx_origin;           ///< The transaction origin account.
    struct bvmc_address block_coinbase;      ///< The miner of the block.
    int64_t block_number;                   ///< The block number.
    int64_t block_timestamp;                ///< The block timestamp.
    int64_t block_gas_limit;                ///< The block gas limit.
    struct bvmc_uint256be block_difficulty;  ///< The block difficulty.
};

struct bvmc_context;

/// Get transaction context callback function.
///
/// This callback function is used by an BVM to retrieve the transaction and
/// block context.
///
/// @param[out] result   The returned transaction context.
///                      @see ::bvmc_tx_context.
/// @param      context  The pointer to the Host execution context.
///                      @see ::bvmc_context.
typedef void (*bvmc_get_tx_context_fn)(struct bvmc_tx_context* result,
                                       struct bvmc_context* context);

/// Get block hash callback function.
///
/// This callback function is used by an BVM to query the block hash of
/// a given block.
///
/// @param[out] result   The returned block hash value.
/// @param      context  The pointer to the Host execution context.
/// @param      number   The block number. Must be a value between
//                       (and including) 0 and 255.
typedef void (*bvmc_get_block_hash_fn)(struct bvmc_uint256be* result,
                                       struct bvmc_context* context,
                                       int64_t number);

/// The execution status code.
enum bvmc_status_code {
    BVMC_SUCCESS = 0,               ///< Execution finished with success.
    BVMC_FAILURE = 1,               ///< Generic execution failure.
    BVMC_OUT_OF_GAS = 2,
    BVMC_UNDEFINED_INSTRUCTION = 3, ///< Unknown instruction encountered by the VM.
    BVMC_BAD_JUMP_DESTINATION = 4,
    BVMC_STACK_OVERFLOW = 5,
    BVMC_STACK_UNDERFLOW = 6,
    BVMC_REVERT = 7,                ///< Execution terminated with REVERT opcode.

    /// Tried to execute an operation which is restricted in static mode.
    BVMC_STATIC_MODE_VIOLATION = 8,

    /// The dedicated INVALID instruction was hit.
    BVMC_INVALID_INSTRUCTION = 9,

    /// Tried to read outside memory bounds.
    ///
    /// An example is RETURNDATACOPY reading past the available buffer.
    BVMC_INVALID_MEMORY_ACCESS = 10,

    /// Exceptions produced by precompiles/system contracts
    ///
    /// An example: elliptic curve functions handed invalid EC points
    BVMC_PRECOMPILE_FAILURE = 11,

    /// Call depth exceded (if there is a call depth limit)
    BVMC_CALL_DEPTH_EXCEDED = 12,

    /// The BVM rejected the execution of the given code or message.
    ///
    /// This error SHOULD be used to signal that the BVM is not able to or
    /// willing to execute the given code type or message.
    /// If an BVM returns the ::BVMC_REJECTED status code,
    /// the Client MAY try to execute it in other BVM implementation.
    /// For example, the Client tries running a code in the BVM 1.5. If the
    /// code is not supported there, the execution falls back to the BVM 1.0.
    BVMC_REJECTED = -1,

    /// BVM implementation internal error.
    ///
    /// @todo We should rethink reporting internal errors. One of the options
    ///       it to allow using any negative value to represent internal errors.
    BVMC_INTERNAL_ERROR = -2,
};

struct bvmc_result;  ///< Forward declaration.

/// Releases resources assigned to an execution result.
///
/// This function releases memory (and other resources, if any) assigned to the
/// specified execution result making the result object invalid.
///
/// @param result  The execution result which resource are to be released. The
///                result itself it not modified by this function, but becomes
///                invalid and user should discard it as well.
typedef void (*bvmc_release_result_fn)(const struct bvmc_result* result);

/// The BVM code execution result.
struct bvmc_result {
    /// The execution status code.
    enum bvmc_status_code status_code;

    /// The amount of gas left after the execution.
    ///
    /// If bvmc_result::code is not ::BVMC_SUCCESS nor ::BVMC_REVERT
    /// the value MUST be 0.
    int64_t gas_left;

    /// The reference to output data.
    ///
    /// The output contains data coming from RETURN opcode (iff bvmc_result::code
    /// field is ::BVMC_SUCCESS) or from REVERT opcode.
    ///
    /// The memory containing the output data is owned by BVM and has to be
    /// freed with bvmc_result::release().
    ///
    /// This MAY be NULL.
    const uint8_t* output_data;

    /// The size of the output data.
    ///
    /// If output_data is NULL this MUST be 0.
    size_t output_size;

    /// The pointer to a function releasing all resources associated with
    /// the result object.
    ///
    /// This function pointer is optional (MAY be NULL) and MAY be set by
    /// the BVM implementation. If set it MUST be used by the user to
    /// release memory and other resources associated with the result object.
    /// After the result resources are released the result object
    /// MUST NOT be used any more.
    ///
    /// The suggested code pattern for releasing BVM results:
    /// @code
    /// struct bvmc_result result = ...;
    /// if (result.release)
    ///     result.release(&result);
    /// @endcode
    ///
    /// @note
    /// It works similarly to C++ virtual destructor. Attaching the release
    /// function to the result itself allows BVM composition.
    bvmc_release_result_fn release;

    /// The address of the contract created by CREATE opcode.
    ///
    /// This field has valid value only if the result describes successful
    /// CREATE (bvmc_result::status_code is ::BVMC_SUCCESS).
    struct bvmc_address create_address;

    /// Reserved data that MAY be used by a bvmc_result object creator.
    ///
    /// This reserved 4 bytes tog with 20 bytes from create_address form
    /// 24 bytes of memory called "optional data" within bvmc_result struct
    /// to be optionally used by the bvmc_result object creator.
    ///
    /// @see bvmc_result_optional_data, bvmc_get_optional_data().
    ///
    /// Also extends the size of the bvmc_result to 64 bytes (full cache line).
    uint8_t padding[4];
};


/// The union representing bvmc_result "optional data".
///
/// The bvmc_result struct contains 24 bytes of optional data that can be
/// reused by the object creator if the object does not contain
/// bvmc_result::create_address.
///
/// An BVM implementation MAY use this memory to keep additional data
/// when returning result from ::bvmc_execute_fn.
/// The host application MAY use this memory to keep additional data
/// when returning result of performed calls from ::bvmc_call_fn.
///
/// @see bvmc_get_optional_data(), bvmc_get_const_optional_data().
union bvmc_result_optional_data
{
    uint8_t bytes[24];  ///< 24 bytes of optional data.
    void* pointer;      ///< Optional pointer.
};

/// Provides read-write access to bvmc_result "optional data".
static inline union bvmc_result_optional_data* bvmc_get_optional_data(
    struct bvmc_result* result)
{
    return (union bvmc_result_optional_data*) &result->create_address;
}

/// Provides read-only access to bvmc_result "optional data".
static inline const union bvmc_result_optional_data* bvmc_get_const_optional_data(
    const struct bvmc_result* result)
{
    return (const union bvmc_result_optional_data*) &result->create_address;
}


/// Check account existence callback function
///
/// This callback function is used by the BVM to check if
/// there exists an account at given address.
/// @param      context  The pointer to the Host execution context.
///                      @see ::bvmc_context.
/// @param      address  The address of the account the query is about.
/// @return              1 if exists, 0 otherwise.
typedef int (*bvmc_account_exists_fn)(struct bvmc_context* context,
                                      const struct bvmc_address* address);

/// Get storage callback function.
///
/// This callback function is used by an BVM to query the given contract
/// storage entry.
/// @param[out] result   The returned storage value.
/// @param      context  The pointer to the Host execution context.
///                      @see ::bvmc_context.
/// @param      address  The address of the contract.
/// @param      key      The index of the storage entry.
typedef void (*bvmc_get_storage_fn)(struct bvmc_uint256be* result,
                                    struct bvmc_context* context,
                                    const struct bvmc_address* address,
                                    const struct bvmc_uint256be* key);

/// Set storage callback function.
///
/// This callback function is used by an BVM to update the given contract
/// storage entry.
/// @param context  The pointer to the Host execution context.
///                 @see ::bvmc_context.
/// @param address  The address of the contract.
/// @param key      The index of the storage entry.
/// @param value    The value to be stored.
typedef void (*bvmc_set_storage_fn)(struct bvmc_context* context,
                                    const struct bvmc_address* address,
                                    const struct bvmc_uint256be* key,
                                    const struct bvmc_uint256be* value);

/// Get balance callback function.
///
/// This callback function is used by an BVM to query the balance of the given
/// address.
/// @param[out] result   The returned balance value.
/// @param      context  The pointer to the Host execution context.
///                      @see ::bvmc_context.
/// @param      address  The address.
typedef void (*bvmc_get_balance_fn)(struct bvmc_uint256be* result,
                                    struct bvmc_context* context,
                                    const struct bvmc_address* address);

/// Get code size callback function.
///
/// This callback function is used by an BVM to get the size of the code stored
/// in the account at the given address. For accounts not having a code, this
/// function returns 0.
typedef size_t (*bvmc_get_code_size_fn)(struct bvmc_context* context,
                                        const struct bvmc_address* address);

/// Copy code callback function.
///
/// This callback function is used by an BVM to request a copy of the code
/// of the given account to the memory buffer provided by the BVM.
/// The Client MUST copy the requested code, starting with the given offset,
/// to the provided memory buffer up to the size of the buffer or the size of
/// the code, whichever is smaller.
///
/// @param context      The pointer to the Client execution context.
///                          @see ::bvmc_context.
/// @param address      The address of the account.
/// @param code_offset  The offset of the code to copy.
/// @param buffer_data  The pointer to the memory buffer allocated by the BVM
///                     to store a copy of the requested code.
/// @param buffer_size  The size of the memory buffer.
/// @return             The number of bytes copied to the buffer by the Client.
typedef size_t (*bvmc_copy_code_fn)(struct bvmc_context* context,
                                    const struct bvmc_address* address,
                                    size_t code_offset,
                                    uint8_t* buffer_data,
                                    size_t buffer_size);

/// Selfdestruct callback function.
///
/// This callback function is used by an BVM to SELFDESTRUCT given contract.
/// The execution of the contract will not be stopped, that is up to the BVM.
///
/// @param context      The pointer to the Host execution context.
///                     @see ::bvmc_context.
/// @param address      The address of the contract to be selfdestructed.
/// @param beneficiary  The address where the remaining  is going to be
///                     transferred.
typedef void (*bvmc_selfdestruct_fn)(struct bvmc_context* context,
                                     const struct bvmc_address* address,
                                     const struct bvmc_address* beneficiary);

/// Log callback function.
///
/// This callback function is used by an BVM to inform about a LOG that happened
/// during an BVM bytecode execution.
/// @param context       The pointer to the Host execution context.
///                      @see ::bvmc_context.
/// @param address       The address of the contract that generated the log.
/// @param data          The pointer to unindexed data attached to the log.
/// @param data_size     The length of the data.
/// @param topics        The pointer to the array of topics attached to the log.
/// @param topics_count  The number of the topics. Valid values are between
///                      0 and 4 inclusively.
typedef void (*bvmc_emit_log_fn)(struct bvmc_context* context,
                                 const struct bvmc_address* address,
                                 const uint8_t* data,
                                 size_t data_size,
                                 const struct bvmc_uint256be topics[],
                                 size_t topics_count);

/// Pointer to the callback function supporting BVM calls.
///
/// @param[out] result  The result of the call. The result object is not
///                     initialized by the BVM, the Client MUST correctly
///                     initialize all expected fields of the structure.
/// @param      context The pointer to the Host execution context.
///                     @see ::bvmc_context.
/// @param      msg     Call parameters. @see ::bvmc_message.
typedef void (*bvmc_call_fn)(struct bvmc_result* result,
                             struct bvmc_context* context,
                             const struct bvmc_message* msg);

/// The context interface.
///
/// The set of all callback functions expected by BVM instances. This is C
/// realisation of vtable for OOP interface (only virtual methods, no data).
/// Host implementations SHOULD create constant singletons of this (similarly
/// to vtables) to lower the maintenance and memory management cost.
struct bvmc_context_fn_table {

    /// Check account existence callback function.
    bvmc_account_exists_fn account_exists;

    /// Get storage callback function.
    bvmc_get_storage_fn get_storage;

    /// Set storage callback function.
    bvmc_set_storage_fn set_storage;

    /// Get balance callback function.
    bvmc_get_balance_fn get_balance;

    /// Get code size callback function.
    bvmc_get_code_size_fn get_code_size;

    /// Copy code callback function.
    bvmc_copy_code_fn copy_code;

    /// Selfdestruct callback function.
    bvmc_selfdestruct_fn selfdestruct;

    /// Call callback function.
    bvmc_call_fn call;

    /// Get transaction context callback function.
    bvmc_get_tx_context_fn get_tx_context;

    /// Get block hash callback function.
    bvmc_get_block_hash_fn get_block_hash;

    /// Emit log callback function.
    bvmc_emit_log_fn emit_log;
};


/// Execution context managed by the Host.
///
/// The Host MUST pass the pointer to the execution context to
/// ::bvmc_execute_fn. The BVM MUST pass the same pointer back to the Host in
/// every callback function.
/// The context MUST contain at least the function table defining the context
/// callback interface.
/// Optionally, The Host MAY include in the context additional data.
struct bvmc_context {

    /// Function table defining the context interface (vtable).
    const struct bvmc_context_fn_table* fn_table;
};


struct bvmc_instance;  ///< Forward declaration.

/// Destroys the BVM instance.
///
/// @param bvm  The BVM instance to be destroyed.
typedef void (*bvmc_destroy_fn)(struct bvmc_instance* bvm);


/// Configures the BVM instance.
///
/// Allows modifying options of the BVM instance.
/// Options:
/// - code cache behavior: on, off, read-only, ...
/// - optimizations,
///
/// @param bvm    The BVM instance to be configured.
/// @param name   The option name. NULL-terminated string. Cannot be NULL.
/// @param value  The new option value. NULL-terminated string. Cannot be NULL.
/// @return       1 if the option set successfully, 0 otherwise.
typedef int (*bvmc_set_option_fn)(struct bvmc_instance* bvm,
                                  char const* name,
                                  char const* value);


/// BVM revision.
///
/// The revision of the BVM specification based on the eum
/// upgrade / hard fork codenames.
enum bvmc_revision {
    BVMC_FRONTIER = 0,
    BVMC_HOMESTEAD = 1,
    BVMC_TANGERINE_WHISTLE = 2,
    BVMC_SPURIOUS_DRAGON = 3,
    BVMC_BYZANTIUM = 4,
    BVMC_CONSTANTINOPLE = 5,
};


/// Generates and executes machine code for given BVM bytecode.
///
/// All the fun is here. This function actually does something useful.
///
/// @param instance    A BVM instance.
/// @param context     The pointer to the Host execution context to be passed
///                    to callback functions. @see ::bvmc_context.
/// @param rev         Requested BVM specification revision.
/// @param msg         Call parameters. @see ::bvmc_message.
/// @param code        Reference to the bytecode to be executed.
/// @param code_size   The length of the bytecode.
/// @return            All execution results.
typedef struct bvmc_result (*bvmc_execute_fn)(struct bvmc_instance* instance,
                                             struct bvmc_context* context,
                                             enum bvmc_revision rev,
                                             const struct bvmc_message* msg,
                                             uint8_t const* code,
                                             size_t code_size);


/// The BVM instance.
///
/// Defines the base struct of the BVM implementation.
struct bvmc_instance
{
    /// BVMC ABI version implemented by the BVM instance.
    ///
    /// Used to detect ABI incompatibilities. The BVMC ABI version
    /// represented by this file is in ::BVMC_ABI_VERSION.
    const int abi_version;

    /// The name of the BVMC VM implementation.
    ///
    /// It MUST be a NULL-terminated not empty string.
    const char* name;

    /// The version of the BVMC VM implementation, e.g. "1.2.3b4".
    ///
    /// It MUST be a NULL-terminated not empty string.
    const char* version;

    /// Pointer to function destroying the BVM instance.
    bvmc_destroy_fn destroy;

    /// Pointer to function executing a code by the BVM instance.
    bvmc_execute_fn execute;

    /// Optional pointer to function modifying VM's options.
    ///
    /// If the VM does not support this feature the pointer can be NULL.
    bvmc_set_option_fn set_option;
};

// END Python CFFI declarations

/// Example of a function creating an instance of an example BVM implementation.
///
/// Each BVM implementation MUST provide a function returning an BVM instance.
/// The function SHOULD be named `bvmc_create_<vm-name>(void)`.
///
/// @return  BVM instance or NULL indicating instance creation failure.
///
/// struct bvmc_instance* bvmc_create_examplbvm(void);


#if __cplusplus
}
#endif

#endif  // BVMC_H
/// @}
