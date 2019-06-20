//
// Created by friday on 20/06/2019.
//


#include <boost/program_options.hpp>
#include <iostream>
namespace bpo = boost::program_options;
int main(int argc, char *argv[]){

    bpo::options_description desc;
    desc.add_options()("help, h", "print help.")
            ("seed", bpo::value<std::string>(), "seed private key.")
            ("node", bpo::value<std::string>(), "node server rpc.");

    bpo::variables_map vm;
    bpo::parsed_options parsed = bpo::parse_command_line(argc, argv, desc);
    bpo::store(parsed, vm);
    bpo::notify(vm);

    if(vm.count("help")){
        std::cout << desc << std::endl;
        return 0;
    }








    return 0;
}