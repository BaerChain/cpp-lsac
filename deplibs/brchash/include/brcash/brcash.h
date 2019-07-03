/* brcash: C/C++ implementation of Brcash, the brcd Proof of Work algorithm.
 * Copyright 2018 Pawel Bylica.
 * Licensed under the Apache License, Version 2.0. See the LICENSE file.
 */

#pragma once

#include <brcash/hash_types.h>

#include <stdint.h>

#ifdef __cplusplus
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define BRCASH_EPOCH_LENGTH 30000
#define BRCASH_LIGHT_CACHE_ITEM_SIZE 64
#define BRCASH_FULL_DATASET_ITEM_SIZE 128
#define BRCASH_NUM_DATASET_ACCESSES 64


struct brcash_epoch_context
{
    const int epoch_number;
    const int light_cache_num_items;
    const union brcash_hash512* const light_cache;
    const int full_dataset_num_items;
};


struct brcash_epoch_context_full;


/**
 * Calculates the number of items in the light cache for given epoch.
 *
 * This function will search for a prime number matching the criteria given
 * by the Brcash so the execution time is not constant. It takes ~ 0.01 ms.
 *
 * @param epoch_number  The epoch number.
 * @return              The number items in the light cache.
 */
int brcash_calculate_light_cache_num_items(int epoch_number) NOEXCEPT;


/**
 * Calculates the number of items in the full dataset for given epoch.
 *
 * This function will search for a prime number matching the criteria given
 * by the Brcash so the execution time is not constant. It takes ~ 0.05 ms.
 *
 * @param epoch_number  The epoch number.
 * @return              The number items in the full dataset.
 */
int brcash_calculate_full_dataset_num_items(int epoch_number) NOEXCEPT;

/**
 * Calculates the epoch seed hash.
 * @param epoch_number  The epoch number.
 * @return              The epoch seed hash.
 */
union brcash_hash256 brcash_calculate_epoch_seed(int epoch_number) NOEXCEPT;


struct brcash_epoch_context* brcash_create_epoch_context(int epoch_number) NOEXCEPT;

/**
 * Creates the epoch context with the full dataset initialized.
 *
 * The memory for the full dataset is only allocated and marked as "not-generated".
 * The items of the full dataset are generated on the fly when hit for the first time.
 *
 * The memory allocated in the context MUST be freed with brcash_destroy_epoch_context_full().
 *
 * @param epoch_number  The epoch number.
 * @return  Pointer to the context or null in case of memory allocation failure.
 */
struct brcash_epoch_context_full* brcash_create_epoch_context_full(int epoch_number) NOEXCEPT;

void brcash_destroy_epoch_context(struct brcash_epoch_context* context) NOEXCEPT;

void brcash_destroy_epoch_context_full(struct brcash_epoch_context_full* context) NOEXCEPT;

#ifdef __cplusplus
}
#endif
