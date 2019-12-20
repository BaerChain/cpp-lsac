#include <libbrccore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/base58.h>
#include <iostream>
using namespace dev;
using namespace dev::brc;

void create_address(){
    auto key_pair = KeyPair::create();
    auto sec = key_pair.secret();
    std::cout << "private-key: " << dev::crypto::to_base58((char*)sec.data(), 32) << std::endl;
    std::cout << "address    : " << key_pair.address() << std::endl;
    std::cout << "public-key : " << toString(key_pair.pub())<<std::endl;
}
/**
 *  private-key: 3wWKfj9Y5KdBkM4fmKZx5whejZdEpErc8adjS65vV6MU
    address    : 7b21d84597d7f47e53bfba1053865ab739357aab
    public-key : aec253626c11e327c53eb4040f2e7c0b116f5884e421fae637ac357e54f82598502b046e367234ab11a908c3fde328f07637e5c996fa3db135c462640419d81c
 *
 *
 * **/
 std::string createNewAddress(std::string const& addr = ""){
     std::string oldAddr = addr;
    if(Address(oldAddr) == Address()){
        std::cout << "error address ...  will create new address"<<std::endl;
        auto key_pair = KeyPair::create();
        auto sec = key_pair.secret();
        std::cout << "private-key: " << dev::crypto::to_base58((char*)sec.data(), 32) << std::endl;
        std::cout << "address    : " << key_pair.address() << std::endl;
        std::cout << "public-key : " << toString(key_pair.pub())<<std::endl;
        oldAddr = toString(key_pair.address());
    }
    std::string new_addr = "brc";
    auto _hash = dev::sha3(oldAddr);
    std::cout << _hash<<"\n";
    std::cout << _hash.hex();
 }
int main(int argc, char *argv[]){

    std::string a1 ="";
    // brc adderss : brc****########
    //create_address();
    return 0;
}