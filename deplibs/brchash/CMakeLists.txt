


set(sources
        include/ethash/ethash.h
        include/ethash/hash_types.h
        include/ethash/keccak.h
        include/ethash/ethash.hpp
        include/ethash/hash_types.hpp
        include/ethash/keccak.hpp
        src/primes.h
        src/endianness.hpp
        src/ethash-internal.hpp
        src/keccak.c
        src/keccakf1600.c
        src/primes.c
        src/ethash.cpp
        src/managed.cpp
        )

add_library(brchash ${sources})
target_include_directories(brchash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)