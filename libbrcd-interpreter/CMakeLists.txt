set(
    sources
    interpreter.h
    VM.cpp
    VM.h
    VMCalls.cpp
    VMConfig.h
    VMOpt.cpp
)
add_library(brcd-interpreter STATIC ${sources})
target_link_libraries(brcd-interpreter PRIVATE devcore brcd-buildinfo loader instructions )
target_include_directories(brcd-interpreter PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/bvmc/include)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(brcd-interpreter PRIVATE -fstack-usage)
endif()
