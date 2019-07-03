add_subdirectory(chainbase)
add_subdirectory(database)


if(${BUILD_TEST} STREQUAL "ON")
    add_subdirectory(test)
endif()