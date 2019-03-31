set(sources
    DposData.h
    Common.h
    Common.cpp
    Dpos.cpp
    Dpos.h
    DposClient.cpp
    DposClient.h
    DposHostCapability.cpp
    DposHostCapability.h
)

add_library(dposseal ${sources})
target_include_directories(dposseal PRIVATE ../utils)
target_link_libraries(dposseal PUBLIC brcdchain p2p brccore devcore)