
set(sources
        bvmc.cpp BVMC.h
        ExtVMFace.cpp ExtVMFace.h
        Instruction.cpp Instruction.h
        LegacyVM.cpp LegacyVM.h
        LegacyVMConfig.h
        LegacyVMCalls.cpp
        LegacyVMOpt.cpp
        VMFace.h
        VMFactory.cpp VMFactory.h
        )

add_library(libvm STATIC  ${sources})

target_link_libraries(
        libvm
        PUBLIC brccore devcore instructions loader
        PRIVATE brcd-interpreter brcd-buildinfo jsoncpp_lib_static Boost::program_options
)

target_include_directories(libvm PUBLIC ${CMAKE_SOURCE_DIR}/bvmc/include)

#if(BRC_OPTIMIZE)
#    target_compile_definitions(libvm PRIVATE BRC_OPTIMIZE)
#endif()
