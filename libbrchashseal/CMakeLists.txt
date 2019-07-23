set(sources
    Brchash.cpp
    Brchash.h
    BrchashClient.cpp
    BrchashClient.h
    BrchashCPUMiner.cpp
    BrchashCPUMiner.h
    BrchashProofOfWork.cpp
    BrchashProofOfWork.h
    GenesisInfo.cpp
    GenesisInfo.h
)

add_library(brchashseal ${sources})
target_include_directories(brchashseal PRIVATE ../utils ${CMAKE_SOURCE_DIR}
        "${CMAKE_SOURCE_DIR}/indexDb/database/include"
        "${CMAKE_SOURCE_DIR}/indexDb/chainbase/include")
target_link_libraries(brchashseal PUBLIC brcdchain brccore)




