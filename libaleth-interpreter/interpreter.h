#pragma once

#include <evmc/evmc.h>
#include <evmc/utils.h>

#if __cplusplus
extern "C" {
#endif

EVMC_EXPORT struct evmc_instance* evmc_create_interpreter() EVMC_NOEXCEPT;

#if __cplusplus
}
#endif
