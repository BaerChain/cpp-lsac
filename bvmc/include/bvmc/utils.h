#pragma once
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
