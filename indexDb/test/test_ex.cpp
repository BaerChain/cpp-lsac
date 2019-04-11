//
// Created by fri on 2019/4/11.
//

#include <stdio.h>
#include <iostream>
#include <boost/filesystem.hpp>
#include <brc/exchangeOrder.hpp>
namespace bfs = boost::filesystem;


int main(int argc, char *argv[]){

    bfs::path cur_dir = bfs::current_path()  ;
    cur_dir /= bfs::path("data");
    cur_dir /= bfs::unique_path();

    dev::brc::ex::exchange_plugin db(cur_dir);




    return 0;
}