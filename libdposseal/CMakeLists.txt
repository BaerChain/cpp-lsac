set(sources
    Dpos.cpp
    Dpos.h
    DposClient.cpp
    DposClient.h
    DposVoteState.h
    DposVoteState.cpp
    DposHostCapability.h
    DposHostCapability.cpp
)

add_library(dposseal ${sources})
target_include_directories(dposseal PRIVATE ../utils)
target_link_libraries(dposseal PUBLIC ethereum p2p ethcore devcore)