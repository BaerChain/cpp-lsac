set(sources
    Common.h
    Common.cpp
    SHDpos.cpp
    SHDpos.h
    SHDposClient.cpp
    SHDposClient.h
    SHDposHostCapability.cpp
    SHDposHostCapability.h
)

add_library(shdposseal ${sources})
target_include_directories(shdposseal PRIVATE ../utils ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/evmc/include)
target_link_libraries(shdposseal PUBLIC brcdchain p2p brccore devcore)
