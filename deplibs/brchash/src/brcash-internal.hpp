// brcash: C/C++ implementation of Brcash, the brcd Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

/// @file
/// Contains declarations of internal brcash functions to allow them to be
/// unit-tested.

#pragma once

#include <brcash/brcash.hpp>

#include "endianness.hpp"

#include <memory>
#include <vector>

extern "C" struct brcash_epoch_context_full : brcash_epoch_context
{
    brcash_hash1024* full_dataset;

    constexpr brcash_epoch_context_full(int epoch_number, int light_cache_num_items,
        const brcash_hash512* light_cache, int full_dataset_num_items,
        brcash_hash1024* full_dataset) noexcept
      : brcash_epoch_context{epoch_number, light_cache_num_items, light_cache,
            full_dataset_num_items},
        full_dataset{full_dataset}
    {}
};

namespace brcash
{
inline bool is_less_or_equal(const hash256& a, const hash256& b) noexcept
{
    for (size_t i = 0; i < (sizeof(a) / sizeof(a.words[0])); ++i)
    {
        if (from_be(a.words[i]) > from_be(b.words[i]))
            return false;
        if (from_be(a.words[i]) < from_be(b.words[i]))
            return true;
    }
    return true;
}

void build_light_cache(hash512 cache[], int num_items, const hash256& seed) noexcept;

hash1024 calculate_dataset_item(const epoch_context& context, uint32_t index) noexcept;

}  // namespace brcash
