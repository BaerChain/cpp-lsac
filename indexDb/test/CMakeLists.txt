



add_executable(test_brc_db db_test.cpp )
target_link_libraries( test_brc_db  ${Boost_LIBRARIES} brc_db devcore)
target_include_directories(test_brc_db
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/include
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}
        )



add_executable(test_ex test_ex.cpp)
target_link_libraries( test_ex  ${Boost_LIBRARIES} brc_db devcore)
target_include_directories(test_ex
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/include
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}
        )
