add_executable(test_pack_tx pack_tx.cpp )
target_link_libraries( test_pack_tx  ${Boost_LIBRARIES} brc_db devcore devcrypto brcdchain)
target_include_directories(test_pack_tx
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/include
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}
        )