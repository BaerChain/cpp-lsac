//
// Created by friday on 28/04/2019.
//


#include <buildinfo.h>

#include <iostream>

int main(){
    auto info = brcd_get_buildinfo();
    std::cout << "project_name: " << info->project_name << std::endl;
    std::cout << "project_version: " << info->project_version << std::endl;
    std::cout << "git_commit_hash: " << info->git_commit_hash << std::endl;
    std::cout << "system_name: " << info->system_name << std::endl;
    std::cout << "system_processor: " << info->system_processor << std::endl;
    std::cout << "compiler_id: " << info->compiler_id << std::endl;
    std::cout << "compiler_version: " << info->compiler_version << std::endl;
    std::cout << "build_type: " << info->build_type << std::endl;


}