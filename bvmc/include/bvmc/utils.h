/* BVMC: Ethereum Client-VM Connector API.
 * Copyright 2018-2019 The BVMC Authors.
 * Licensed under the Apache License, Version 2.0.
 */

#pragma once

/**
 * @file
 * A collection of helper macros to handle some non-portable features of C/C++ compilers.
 *
 * @addtogroup helpers
 * @{
 */

/**
 * @def BVMC_EXPORT
 * Marks a function to be exported from a shared library.
 */
#if defined _MSC_VER || defined __MINGW32__
#define BVMC_EXPORT __declspec(dllexport)
#else
#define BVMC_EXPORT __attribute__((visibility("default")))
#endif

/**
 * @def BVMC_NOEXCEPT
 * Safe way of marking a function with `noexcept` C++ specifier.
 */
#if __cplusplus
#define BVMC_NOEXCEPT noexcept
#else
#define BVMC_NOEXCEPT
#endif

/** @} */
