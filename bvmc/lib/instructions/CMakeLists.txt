add_library(
        instructions STATIC
        ${include_dir}/bvmc/instructions.h
        instruction_metrics.c
        instruction_names.c
)
target_include_directories(instructions PUBLIC ${include_dir})
