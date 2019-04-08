# EVMC: Ethereum Client-VM Connector API.
# Copyright 2018 The EVMC Authors.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

add_library(
        instructions STATIC
        ${include_dir}/evmc/instructions.h
        instruction_metrics.c
        instruction_names.c
)
target_include_directories(instructions PUBLIC ${include_dir})
