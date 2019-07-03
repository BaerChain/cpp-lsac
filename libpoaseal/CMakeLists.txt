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
target_include_directories(poaseal PRIVATE ../utils ${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/indexDb/database/include"
		"${CMAKE_SOURCE_DIR}/indexDb/chainbase/include")
target_link_libraries(poaseal PUBLIC brcdchain brccore)