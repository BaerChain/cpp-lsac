

add_executable(version_test ver.cpp)
target_include_directories(version_test PUBLIC ${CMAKE_SOURCE_DIR}/brcd)
target_link_libraries(version_test PUBLIC brcd-buildinfo)