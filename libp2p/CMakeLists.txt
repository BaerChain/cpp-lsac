#file(GLOB sources "*.cpp" "*.h")



set(sources All.h
        Capability.h
        CapabilityHost.h
        Common.h
        Host.h
        Network.h
        NodeTable.h
        Peer.h
        RLPXFrameCoder.h
        RLPXPacket.h
        RLPXSocket.h
        RLPxHandshake.h
        Session.h
        UDP.h
        UPnP.h
        CapabilityHost.cpp
        Common.cpp
        Host.cpp
        Network.cpp
        NodeTable.cpp
        Peer.cpp
        RLPXFrameCoder.cpp
        RLPxHandshake.cpp
        Session.cpp
        UDP.cpp
        UPnP.cpp

        )
add_library(p2p ${sources})
target_link_libraries(p2p PUBLIC devcrypto devcore)
target_include_directories(p2p SYSTEM PRIVATE ${CRYPTOPP_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

if(MINIUPNPC)
    target_link_libraries(p2p PRIVATE "${MINIUPNPC_LIBRARY}")
    target_include_directories(p2p SYSTEM PRIVATE "${MINIUPNPC_INCLUDE_DIR}")
    target_compile_definitions(p2p PRIVATE BRC_MINIUPNPC)
endif()