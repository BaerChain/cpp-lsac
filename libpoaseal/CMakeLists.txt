set(sources
    Common.h
    Poa.cpp
    Poa.h
    PoaClient.cpp
    PoaClient.h
	PoaHostCapability.cpp
	PoaHostCapability.h
)

add_library(poaseal ${sources})
target_include_directories(poaseal PRIVATE ../utils)
target_link_libraries(poaseal PUBLIC ethereum ethcore)