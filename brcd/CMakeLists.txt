set(
    sources
    AccountManager.cpp AccountManager.h
    main.cpp
    MinerAux.cpp MinerAux.h
)

#set(CMAKE_CXX_LINK_EXECUTABLE  "-static")
#message(STATUS "CMAKE_CXX_LINK_EXECUTABLE = " ${CMAKE_CXX_LINK_EXECUTABLE})
#add_definitions(-DBOOST_LOG_DYN_LINK)
add_executable(brcd ${sources})

target_link_libraries(
    brcd
    PRIVATE brcdchain  brchashseal poaseal shdposseal loader instructions web3jsonrpc webthree devcore ${Boost_LIBRARIES} libvm
	PUBLIC ${MHD_LIBRARIES} ${GMP_LIBRARIES} ${Boost_LIBRARIES}  dl
    )

target_include_directories(brcd PRIVATE ../utils PUBLIC ${MHD_INCLUDE_DIRS}  )

if(MINIUPNPC)
    target_compile_definitions(brcd PRIVATE BRC_MINIUPNPC )
endif()

install(TARGETS brcd DESTINATION bin)




execute_process(
        COMMAND	git log -1 --format=%H
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_VERSION
)

string (REGEX REPLACE "[\n\t\r]" "" GIT_VERSION  ${GIT_VERSION})

add_library(brcd-buildinfo buildinfo.c buildinfo.h)

message(STATUS "build with git version : ${GIT_VERSION}")

target_compile_definitions(brcd-buildinfo PUBLIC
        -DGIT_VERSION="${GIT_VERSION}"
        -DSYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
        -DSYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}"
        -DCOMPILER_ID="${CMAKE_CXX_COMPILER_ID}"
        -DBUILD_TYPE="${CMAKE_BUILD_TYPE}"
        -DPROJECT_NAME="${CMAKE_PROJECT_NAME}"
        -DBUILD_COMPILER_ID="${CMAKE_CXX_COMPILER}"
        -DBUILD_COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}"
        -DBUILD_PROJECT_NAME="${CMAKE_PROJECT_NAME}"
        )







