



#set(COMPILE_OPTIONS -O3 -W -std=c89 -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings -fvisibility=hidden)

add_executable(pub_key_test
        eccTest.cpp

        )
target_link_libraries( pub_key_test  ${Boost_LIBRARIES} devcrypto devcore ethereum web3jsonrpc ${OPENSSL_LIBRARIES})

target_include_directories(pub_key_test
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        )
