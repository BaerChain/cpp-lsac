#pragma once

#include <bvmc/bvmc.h>

/**
 * Returns true if the VM instance has a compatible ABI version.
 */
static inline int bvmc_is_abi_compatible(struct bvmc_instance* instance)
{
    return instance->abi_version == BVMC_ABI_VERSION;
}

/**
 * Returns the name of the VM instance.
 */
static inline const char* bvmc_vm_name(struct bvmc_instance* instance)
{
    return instance->name;
}

/**
 * Returns the version of the VM instance.
 */
static inline const char* bvmc_vm_version(struct bvmc_instance* instance)
{
    return instance->version;
}

/**
 * Checks if the VM instance has the given capability.
 *
 * @see bvmc_get_capabilities_fn
 */
static inline bool bvmc_vm_has_capability(struct bvmc_instance* vm,
                                          enum bvmc_capabilities capability)
{
    return (vm->get_capabilities(vm) & (bvmc_capabilities_flagset)capability) != 0;
}

/**
 * Destroys the VM instance.
 *
 * @see bvmc_destroy_fn
 */
static inline void bvmc_destroy(struct bvmc_instance* instance)
{
    instance->destroy(instance);
}

/**
 * Sets the option for the VM instance, if the feature is supported by the VM.
 *
 * @see bvmc_set_option_fn
 */
static inline enum bvmc_set_option_result bvmc_set_option(struct bvmc_instance* instance,
                                                          char const* name,
                                                          char const* value)
{
    if (instance->set_option)
        return instance->set_option(instance, name, value);
    return BVMC_SET_OPTION_INVALID_NAME;
}

/**
 * Sets the tracer callback for the VM instance, if the feature is supported by the VM.
 *
 * @see bvmc_set_tracer_fn
 */
static inline void bvmc_set_tracer(struct bvmc_instance* instance,
                                   bvmc_trace_callback callback,
                                   struct bvmc_tracer_context* context)
{
    if (instance->set_tracer)
        instance->set_tracer(instance, callback, context);
}

/**
 * Executes code in the VM instance.
 *
 * @see bvmc_execute_fn.
 */
static inline struct bvmc_result bvmc_execute(struct bvmc_instance* instance,
                                              struct bvmc_context* context,
                                              enum bvmc_revision rev,
                                              const struct bvmc_message* msg,
                                              uint8_t const* code,
                                              size_t code_size)
{
    return instance->execute(instance, context, rev, msg, code, code_size);
}

/**
 * Releases the resources allocated to the execution result.
 *
 * @see bvmc_release_result_fn
 */
static inline void bvmc_release_result(struct bvmc_result* result)
{
    result->release(result);
}


/**
 * Helpers for optional storage of bvmc_result.
 *
 * In some contexts (i.e. bvmc_result::create_address is unused) objects of
 * type bvmc_result contains a memory storage that MAY be used by the object
 * owner. This group defines helper types and functions for accessing
 * the optional storage.
 *
 * @defgroup result_optional_storage Result Optional Storage
 * @{
 */

/**
 * The union representing bvmc_result "optional storage".
 *
 * The bvmc_result struct contains 24 bytes of optional storage that can be
 * reused by the object creator if the object does not contain
 * bvmc_result::create_address.
 *
 * A VM implementation MAY use this memory to keep additional data
 * when returning result from bvmc_execute_fn().
 * The host application MAY use this memory to keep additional data
 * when returning result of performed calls from bvmc_call_fn().
 *
 * @see bvmc_get_optional_storage(), bvmc_get_const_optional_storage().
 */
union bvmc_result_optional_storage
{
    uint8_t bytes[24]; /**< 24 bytes of optional storage. */
    void* pointer;     /**< Optional pointer. */
};

/** Provides read-write access to bvmc_result "optional storage". */
static inline union bvmc_result_optional_storage* bvmc_get_optional_storage(
    struct bvmc_result* result)
{
    return (union bvmc_result_optional_storage*)&result->create_address;
}

/** Provides read-only access to bvmc_result "optional storage". */
static inline const union bvmc_result_optional_storage* bvmc_get_const_optional_storage(
    const struct bvmc_result* result)
{
    return (const union bvmc_result_optional_storage*)&result->create_address;
}

/** @} */

/** @} */
