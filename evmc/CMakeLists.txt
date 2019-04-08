# EVMC: Ethereum Client-VM Connector API.
# Copyright 2018 The EVMC Authors.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

cmake_minimum_required(VERSION 3.5)

if(TARGET evmc)
    # The evmc library has been already created (probably by other submodule).
    return()
endif()

#option(EVMC_TESTING "Build EVMC tests and test tools" OFF)
#option(EVMC_EXAMPLES "Build EVMC examples" ${EVMC_TESTING})
#option(HUNTER_ENABLED "Enable Hunter package manager support" "${EVMC_TESTING}")


project(evmc)
set(PROJECT_VERSION 6.0.0)

set(include_dir ${PROJECT_SOURCE_DIR}/include)


add_subdirectory(lib)

if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
string(TOLOWER ${CMAKE_SYSTEM_NAME} system_name)
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION}-${system_name})
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)

