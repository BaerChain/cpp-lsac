file(GLOB sources "*.cpp" "*.h")
#add_definitions(-w)
add_library(brcdchain STATIC ${sources})

target_include_directories(brcdchain PRIVATE "${UTILS_INCLUDE_DIR}" ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/bvmc/include)
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
            dl
        )
#target_compile_options(devcore PUBLIC -DBOOST_ALL_DYN_LINK )
