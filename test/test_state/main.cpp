//
// Created by friday on 23/05/2019.
//


#include <libbrcdchain/State.h>

using namespace dev;
using namespace dev::brc;



int main(int argc, char *argv[]){

    std::map<dev::h256, bytesRef> test_data;
    for(auto i = 0; i < 10; i++){
        dev::h256  key = dev::sha3(std::to_string(i));

        std::string value = std::to_string(i) + "test";
        bytesRef data((uint8_t*)value.c_str(), value.size());

        test_data[key] = data;
    }

    auto db = State::openDB(boost::filesystem::unique_path(), dev::h256());
    for(auto itr : test_data){
//        std::cout << "first " << itr.first << " :" << itr.second.toString() << std::endl;
        db.insert(itr.first, itr.second);
        db.commit();
    }

    for(auto i = 0 ; i < test_data.size(); i++){
        for(auto itr : test_data){
            auto ret = db.lookup(itr.first);
            std::cout << itr.first << " " << ret << std::endl;
        }
        db.rollback();
        std::cout << "----------------------------\n";
    }


    return 0;
}
