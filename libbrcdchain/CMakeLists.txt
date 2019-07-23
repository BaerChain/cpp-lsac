file(GLOB sources "*.cpp" "*.h")
add_library(brcdchain STATIC ${sources})

target_include_directories(brcdchain
		PUBLIC
		"${UTILS_INCLUDE_DIR}"
		${CMAKE_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/evmc/include
		PRIVATE
		"${CMAKE_SOURCE_DIR}/indexDb/database/include"
		)

target_link_libraries(brcdchain
        PUBLIC
            loader
            instructions
            brccore
            p2p
            devcrypto
            devcore
			libvm

            jsoncpp_lib_static
            ${SNAPPY_LIBRARIES}
            ${Boost_LIBRARIES}
            ${JSON_RPC_CPP_CLIENT_LIBRARY}
            dl
			brc_db
        )
#target_compile_options(devcore PUBLIC -DBOOST_ALL_DYN_LINK )
