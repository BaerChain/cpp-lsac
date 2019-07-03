cmake_minimum_required(VERSION 3.5)

if(TARGET bvmc)
    return()
endif()

project(bvmc)
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

