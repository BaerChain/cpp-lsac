add_library(webthree WebThree.cpp WebThree.h)
target_include_directories(webthree PRIVATE ../utils ${CMAKE_SOURCE_DIR} )
target_link_libraries(webthree PUBLIC poaseal shdposseal brcdchain p2p devcore)
