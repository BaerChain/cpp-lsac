add_executable(brcd-vm main.cpp)

target_link_libraries(brcd-vm PRIVATE brcdchain evm brchashseal devcore Boost::program_options jsoncpp)

target_include_directories(brcd-vm PRIVATE ../utils)

install(TARGETS brcd-vm EXPORT brcdTargets DESTINATION bin)
