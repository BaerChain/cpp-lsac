#include <libbrccore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/CommonData.h>
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
    try {
        toAddress(addr);
    }
    catch (...){
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
    auto ret = _hash.hex();
    std::cout << ret << "\n";
    std::cout << ret.substr(0 ,4)<< "\n";

    auto ret_base58 =  dev::crypto::to_base58(ret.substr(0,4).c_str(), 4);
    std::cout <<"ret_base58:"<< ret_base58 << "\n";

    auto a_bese58 = dev::crypto::to_base58(addr.c_str(), 40);
    std::cout <<"a_base58:"<< a_bese58 << "\n";
    return new_addr + ret_base58 + a_bese58;
 }

 void checkNewAddress(std::string const& _addr){

 }

 void testBase58(){
     std::string s1 = "0000";
     std::string s2 = "ffff";
     //65535
     for(int i=0; i<= 65535; i++){
         auto  ret1 = dev::toBigEndian(u256(i));
         bytes ret(4);
         toBigEndian(u256(i),ret);
         auto  ret_s = toHex(ret);
         if(ret_s.length() >4){
             ret_s= ret_s.substr(ret_s.length()-4, ret_s.length());
         }
         std:: cout << ret_s <<"\n";
         auto ret2 = dev::crypto::to_base58(ret_s.c_str(), 4);
         if(ret2.length() != 6){
             std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<"\n";
             return;
         }
         else{
             std::cout <<ret2<<std::endl;
         }
     }

     std::cout <<dev::crypto::to_base58(s1.c_str(), 4) << "\n";
     std::cout <<dev::crypto::to_base58(s2.c_str(), 4) << "\n";
 }

int main(int argc, char *argv[]){
     /**
      * private-key: 4N76qbS6YMA4KYbEwmW8CKLnVUtpfkegLPziaJ2zRQmi
        address    : efee5b4be9ba66b2f371d305cd4e0788fa2c7661
        public-key : fb27fe5ce0e83b5541562ed12e32d7e8aeb1fd1a9679267d5541800e4ecaab6d71cdd290205c7a3d945b83cab5af1c80ad9fa1d367b1e099f7dbfbce55c9b351
        3eb6689f1d9925ab60b92ff7614902a88302fd939bf9edb3c2b7c12ef41c75d4

        3eb6
    ret_base58:2KCRV7
    a_base58:63Dsjn8pvqbV3oQDvBRByt1cTtxQSZEfx4o1UJe1dZgqSJQqHk9z5Fn
        brc2KCRV763Dsjn8pvqbV3oQDvBRByt1cTtxQSZEfx4o1UJe1dZgqSJQqHk9z5Fn
      *
      * **/
    std::string a1 ="";
    // brc adderss : brc****########
    //create_address();
    auto new_a= createNewAddress("efee5b4be9ba66b2f371d305cd4e0788fa2c7661");
    std::cout << new_a << "\n";


    if(toAddress("efee5b4be9ba66b2f371d305cd4e0788fa2c766") == Address())
        std::cout << "fase"<<"\n";

    std:: cout << "*************************************" << std::endl;
    testBase58();
    return 0;
}