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
target_link_libraries(brcd-interpreter PRIVATE devcore brcd-buildinfo evmc::evmc evmc::instructions)

if(ALETH_INTERPRETER_SHARED)
    # Build brcd-interpreter additionally as a shared library to include it in the package
    add_library(brcd-interpreter-shared SHARED ${sources})
    target_link_libraries(brcd-interpreter-shared PRIVATE devcore brcd-buildinfo evmc::evmc evmc::instructions)
    set_target_properties(brcd-interpreter-shared PROPERTIES OUTPUT_NAME brcd-interpreter)
    install(TARGETS brcd-interpreter-shared EXPORT brcdTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(brcd-interpreter PRIVATE -fstack-usage)
endif()
