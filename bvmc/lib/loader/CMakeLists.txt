
add_library(
        loader STATIC
        ${include_dir}/bvmc/loader.h
        loader.c
)
target_link_libraries(loader PUBLIC dl)
target_include_directories(loader PUBLIC ${include_dir})

