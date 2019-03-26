set(
    sources
    AccountManager.cpp AccountManager.h
    main.cpp
    MinerAux.cpp MinerAux.h
)

add_executable(aleth ${sources})
target_link_libraries(
    aleth
    PRIVATE ethereum ethashseal poaseal dposseal evm web3jsonrpc webthree devcore Boost::program_options
	PUBLIC ${MHD_LIBRARIES}
    )

target_include_directories(aleth PRIVATE ../utils PUBLIC ${MHD_INCLUDE_DIRS})

if(MINIUPNPC)
    target_compile_definitions(aleth PRIVATE ETH_MINIUPNPC)
endif()

install(TARGETS aleth DESTINATION bin)
