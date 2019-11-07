
add_executable(testTree testTree.cpp treeTest.cpp)
target_link_libraries( testTree  ${Boost_LIBRARIES} brc_db devcore devcrypto brcdchain)
target_include_directories(testTree
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/include
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}
        )