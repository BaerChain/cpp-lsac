add_library(webthree WebThree.cpp WebThree.h)
target_include_directories(webthree PRIVATE ../utils ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/evmc/include)
target_link_libraries(webthree PUBLIC poaseal shdposseal brcdchain p2p devcore)
