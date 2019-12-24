




add_executable(pathrpc main.cpp)
target_include_directories(pathrpc PUBLIC ${CMAKE_SOURCE_DIR}/libweb3jsonrpc)
target_link_libraries(pathrpc PUBLIC web3jsonrpc)