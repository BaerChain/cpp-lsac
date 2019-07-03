# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(include_dir ${PROJECT_SOURCE_DIR}/include)

add_library(
    brcash
    endianness.hpp
    ${include_dir}/brcash/brcash.h
    ${include_dir}/brcash/brcash.hpp
    brcash-internal.hpp
    brcash.cpp
    ${include_dir}/brcash/hash_types.h
    managed.cpp
    ${include_dir}/brcash/keccak.h
    ${include_dir}/brcash/keccak.hpp
    keccak.c
    keccakf1600.c
    primes.h
    primes.c
)

target_include_directories(brcash PUBLIC $<BUILD_INTERFACE:${include_dir}>$<INSTALL_INTERFACE:include>)

write_basic_package_version_file(brcashConfigVersion.cmake COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    ../../cmake/Config.cmake.in
    brcashConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/brcash
)

install(
    TARGETS brcash
    EXPORT brcashTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    DIRECTORY
    ${include_dir}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(BRCASH_INSTALL_CMAKE_CONFIG)
    install(
        EXPORT brcashTargets
        NAMESPACE brcash::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/brcash
    )
    install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/brcashConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/brcashConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/brcash
    )
endif()

get_filename_component(BRCASH_PRIVATE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
set(BRCASH_PRIVATE_INCLUDE_DIR ${BRCASH_PRIVATE_INCLUDE_DIR} PARENT_SCOPE)