set(
    sources
    AccountManager.cpp AccountManager.h
    main.cpp
    MinerAux.cpp MinerAux.h
)

add_executable(brcd ${sources})
target_link_libraries(
    brcd
    PRIVATE brcdchain brchashseal poaseal dposseal evm web3jsonrpc webthree devcore Boost::program_options
	PUBLIC ${MHD_LIBRARIES}
    )

target_include_directories(brcd PRIVATE ../utils PUBLIC ${MHD_INCLUDE_DIRS})

if(MINIUPNPC)
    target_compile_definitions(brcd PRIVATE BRC_MINIUPNPC)
endif()

install(TARGETS brcd DESTINATION bin)
