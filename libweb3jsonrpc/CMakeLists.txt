set(sources
        AccountHolder.cpp
        AccountHolder.h
        AdminBrc.cpp
        AdminBrc.h
        AdminBrcFace.h
        AdminNet.cpp
        AdminNet.h
        AdminNetFace.h
        Debug.cpp
        Debug.h
        DebugFace.h
        Brc.cpp
        Brc.h
        BrcFace.h
        IpcServer.h
        IpcServerBase.cpp
        IpcServerBase.h
        JsonHelper.cpp
        JsonHelper.h
        ModularServer.h
        Net.cpp
        Net.h
        NetFace.h
        Personal.cpp
        Personal.h
        PersonalFace.h
        SessionManager.cpp
        SessionManager.h
        Test.cpp
        Test.h
        TestFace.h
        Web3.cpp
        Web3.h
        Web3Face.h
        WhisperFace.h
        SafeHttpServer.h
        SafeHttpServer.cpp
        BrcV2.h
        BrcV2.cpp
        BrcFace.h
        JsonHelperV2.h
        JsonHelperV2.cpp
        )

if(WIN32)
    list(APPEND sources WinPipeServer.cpp WinPipeServer.h)
else()
    list(APPEND sources UnixSocketServer.cpp UnixSocketServer.h)
endif()

add_library(web3jsonrpc  ${sources})
target_link_libraries(web3jsonrpc PUBLIC
        brchashseal
        webthree
        brchash
        ${JSON_RPC_CPP_SERVER_LIBRARY}
        ${MHD_LIBRARIES}
        ${Boost_LIBRARIES}
        ${JSON_RPC_CPP_COMMON_LIBRARY}
        brc_db
        )
target_include_directories(web3jsonrpc PUBLIC
        ${MHD_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}
        ${Boost_INCLUDE_DIR}
        ${JSON_RPC_CPP_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/indexDb/database/include
        )


