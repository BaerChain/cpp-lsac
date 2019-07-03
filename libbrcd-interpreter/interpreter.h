#pragma once

#include <bvmc/bvmc.h>
#include <bvmc/utils.h>

#if __cplusplus
extern "C" {
#endif

BVMC_EXPORT struct bvmc_instance* bvmc_create_interpreter() BVMC_NOEXCEPT;

#if __cplusplus
}
#endif
