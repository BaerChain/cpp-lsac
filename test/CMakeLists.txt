


add_executable(exchange_test ex_main.cpp)
target_link_libraries( exchange_test  ${Boost_LIBRARIES} devcrypto devcore brcdchain web3jsonrpc ${OPENSSL_LIBRARIES} Boost::program_options ${JSON_RPC_CPP_CLIENT_LIBRARY})

target_include_directories(exchange_test
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/utils
        ${CMAKE_SOURCE_DIR}
        )





add_subdirectory(transaction)
add_subdirectory(version)
add_subdirectory(maxtxs)
add_subdirectory(allTypeTransaction)
add_subdirectory(test_state)
add_subdirectory(test_exchange)
#add_subdirectory(signdata)