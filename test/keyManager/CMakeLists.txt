add_executable(keyMgr main.cpp)
target_link_libraries(keyMgr  ${Boost_LIBRARIES} devcrypto devcore  ${OPENSSL_LIBRARIES} Boost::program_options ${JSON_RPC_CPP_CLIENT_LIBRARY})

target_include_directories(keyMgr
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/utils
        ${CMAKE_SOURCE_DIR}
        )
