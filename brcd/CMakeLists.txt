set(
    sources
    AccountManager.cpp AccountManager.h
    main.cpp
    MinerAux.cpp MinerAux.h
)


#add_definitions(-DBOOST_LOG_DYN_LINK)
add_executable(brcd ${sources})

target_link_libraries(
    brcd
    PRIVATE  brchashseal poaseal shdposseal loader instructions web3jsonrpc webthree devcore ${Boost_LIBRARIES} libvm
	PUBLIC ${MHD_LIBRARIES} ${GMP_LIBRARIES} ${Boost_LIBRARIES} brcdchain dl
    )

target_include_directories(brcd PRIVATE ../utils PUBLIC ${MHD_INCLUDE_DIRS} )

if(MINIUPNPC)
    target_compile_definitions(brcd PRIVATE BRC_MINIUPNPC )
endif()

install(TARGETS brcd DESTINATION bin)




