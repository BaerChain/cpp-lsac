file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(devcrypto ${SOURCES} ${HEADERS})

target_include_directories(devcrypto
        PRIVATE
        ${UTILS_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}
        PUBLIC
        ${OPENSSL_INCLUDE_DIR}
	${JSONCPP_INCLUDE_DIR}
        ${BOOST_INCLUDEDIR}
        ${CMAKE_SOURCE_DIR}/utils
        ${CMAKE_SOURCE_DIR}/deplibs/libff/libff
        ${CMAKE_SOURCE_DIR}/deplibs/libff/
        ${CMAKE_SOURCE_DIR}/deplibs/secp256k1/include
		${CMAKE_SOURCE_DIR}/deplibs
        )
target_link_libraries(devcrypto
        PUBLIC devcore
        ${OPENSSL_LIBRARIES}
        ${Boost_LIBRARIES}
        secp256k1
        cryptopp-static
        ff
        ${LIBSCRYPT_LIBRARIES}
        )



