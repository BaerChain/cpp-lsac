file(GLOB sources "*.cpp" "*.h")
add_definitions(-w)
add_library(brcdchain ${sources})

target_include_directories(brcdchain PRIVATE "${UTILS_INCLUDE_DIR}")
target_link_libraries(brcdchain  PUBLIC evm brccore p2p devcrypto devcore PRIVATE jsoncpp_lib_static Snappy::snappy Boost::fiber Boost::context)
