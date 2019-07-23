


set(sources
        include/brcash/brcash.h
        include/brcash/hash_types.h
        include/brcash/keccak.h
        include/brcash/brcash.hpp
        include/brcash/hash_types.hpp
        include/brcash/keccak.hpp
        src/primes.h
        src/endianness.hpp
        src/brcash-internal.hpp
        src/keccak.c
        src/keccakf1600.c
        src/primes.c
        src/brcash.cpp
        src/managed.cpp
        )

add_library(brchash ${sources})
target_include_directories(brchash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)