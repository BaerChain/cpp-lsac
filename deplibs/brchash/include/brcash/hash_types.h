/* brcash: C/C++ implementation of Brcash, the brcd Proof of Work algorithm.
 * Copyright 2018 Pawel Bylica.
 * Licensed under the Apache License, Version 2.0. See the LICENSE file.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

union brcash_hash256
{
    uint64_t words[4];
    uint32_t hwords[8];
    uint8_t bytes[32];
};

union brcash_hash512
{
    uint64_t words[8];
    uint32_t half_words[16];
    uint8_t bytes[64];
};

union brcash_hash1024
{
    union brcash_hash512 hashes[2];
    uint64_t words[16];
    uint32_t hwords[32];
    uint8_t bytes[128];
};

#ifdef __cplusplus
}
#endif
