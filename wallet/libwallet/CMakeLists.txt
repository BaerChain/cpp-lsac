file(GLOB sources "*.cpp")
file(GLOB headers "*.h")
#add_definitions(-DBOOST_ALL_DYN_LINK)
add_library(walletcore STATIC ${sources} ${headers})

# Needed to prevent including system-level boost headers:
target_link_libraries( walletcore PUBLIC ${Boost_LIBRARIES} devcrypto devcore brcdchain ${JSON_RPC_CPP_CLIENT_LIBRARY})

target_include_directories(walletcore
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${Boost_INCLUDE_DIRS}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/utils
        ${CMAKE_SOURCE_DIR}
        )