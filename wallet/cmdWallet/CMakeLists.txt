


add_executable(cmdWallet main.cpp)
target_link_libraries( cmdWallet  ${Boost_LIBRARIES} devcrypto devcore brcdchain web3jsonrpc ${OPENSSL_LIBRARIES} Boost::program_options ${JSON_RPC_CPP_CLIENT_LIBRARY})

target_include_directories(cmdWallet
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/utils
        ${CMAKE_SOURCE_DIR}
        )
