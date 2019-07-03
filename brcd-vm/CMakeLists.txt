add_executable(brcd-vm main.cpp)

target_link_libraries(brcd-vm PUBLIC brcdchain loader  instructions brchashseal devcore Boost::program_options jsoncpp libvm)

target_include_directories(brcd-vm PRIVATE ../utils ../ ../bvmc/include)

install(TARGETS brcd-vm EXPORT brcdTargets DESTINATION bin)
