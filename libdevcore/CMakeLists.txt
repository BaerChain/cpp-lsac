file(GLOB sources "*.cpp")
file(GLOB headers "*.h")
#add_definitions(-DBOOST_ALL_DYN_LINK)
add_library(devcore STATIC ${sources} ${headers})

# Needed to prevent including system-level boost headers:
target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIR} PRIVATE ../utils ${CMAKE_SOURCE_DIR} ${depenlibs_include_dir})

target_link_libraries(devcore
        PUBLIC
        brchash
        ${Boost_LIBRARIES}
        ${ROCKSDB_LIBRARIES}
        ff
        PRIVATE
        leveldb
        )
#target_compile_options(devcore PUBLIC -DBOOST_ALL_DYN_LINK )

#if (MSVC)
#    find_package(LevelDB)
#    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
#    target_link_libraries(devcore PRIVATE ${LEVELDB_LIBRARIES})
#else()
#endif()

#hunter_add_package(rocksdb)

