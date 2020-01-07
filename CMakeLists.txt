cmake_minimum_required(VERSION 3.10)
project(baerChain VERSION 1.0)


include(ExternalProject)
include(CheckTypeSize)
include(CheckSymbolExists)
set(CMAKE_CXX_STANDARD 14)
#add_definitions(-fvisibility=hidden)




IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DBOOST_LOG_DYN_LINK)
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -Werror -Wreturn-type")
    #SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 ")
endif()




if( APPLE )
    # add_definitions(-DBOOST_LOG_DYN_LINK)
    set(BOOST_LOG_DYN_LINK ON)
    set(Boost_USE_STATIC_LIBS ON)
    set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -stdlib=libc++  -Wno-conversion" )
endif(APPLE)


##add cmake file directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(STATUS "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME} ${APPLE}")


find_package(Boost 1.65.1 REQUIRED COMPONENTS
        unit_test_framework
        thread
        date_time
        system
        filesystem
        program_options
        chrono
        context
        log_setup
        locale
        log
        )

option(BUILD_TEST "build test , ON or OFF" ON)


message(STATUS "BUILD_TEST ${BUILD_TEST}")

find_package(Jsoncpp  REQUIRED)
find_package(MHD REQUIRED)
find_package(Threads)
find_package(OpenSSL 1.0.2 REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Rocksdb  REQUIRED)
find_package(json_rpc_cpp REQUIRED)
find_package(GMP REQUIRED)
find_package(libscrypt  REQUIRED)
find_package(snappy REQUIRED)

message(STATUS "GMP_LIBRARY ${GMP_LIBRARY}")
message(STATUS "GMP_INCLUDE_DIR ${GMP_INCLUDE_DIR}")
message(STATUS "Found jsoncpp : ${JSONCPP_INCLUDE_DIR}")
message(STATUS "Found jsoncpp : ${JSONCPP_LIBRARY}")
message(STATUS "Found MHD_LIBRARIES : ${MHD_LIBRARIES}")



######################  subdirectory ######################
set(depenlibs_dir "${CMAKE_BINARY_DIR}/deplibs/libs/lib")
set(depenlibs_include_dir "${CMAKE_BINARY_DIR}/deplibs/libs/include")



add_subdirectory(deplibs)
add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
add_subdirectory(libbrccore)
add_subdirectory(libweb3jsonrpc)
add_subdirectory(bvmc)
add_subdirectory(libbvm)
add_subdirectory(libbrchashseal)
add_subdirectory(libbrcdchain)
add_subdirectory(libshdposseal)
add_subdirectory(libpoaseal)
add_subdirectory(libwebthree)
add_subdirectory(libbrcd-interpreter)
add_subdirectory(libp2p)
add_subdirectory(brcd-vm)
add_subdirectory(brcd)


add_subdirectory(indexDb)
add_subdirectory(wallet)

if(${BUILD_TEST} STREQUAL "ON")
    add_subdirectory(test)
endif()
