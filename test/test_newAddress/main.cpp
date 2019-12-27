#include <libbrccore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/base58.h>
#include <libbrccore/CommonJS.h>
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
 std::string createNewAddress(std::string const& addr = "") {
    std::string oldAddr = addr;
    try {
        toAddress(addr);
    }
    catch (...) {
        std::cout << "error address ...  will create new address" << std::endl;
        auto key_pair = KeyPair::create();
        auto sec = key_pair.secret();
        std::cout << "private-key: " << dev::crypto::to_base58((char *) sec.data(), 32) << std::endl;
        std::cout << "address    : " << key_pair.address() << std::endl;
        std::cout << "public-key : " << toString(key_pair.pub()) << std::endl;
        oldAddr = toString(key_pair.address());
    }
        std::string new_addr = "brc";
        auto _hash = dev::sha3(Address(oldAddr));
        std::cout << _hash << "\n";

        auto ret_base58 = dev::crypto::to_base58((char *) _hash.data(), 4);
        std::cout << "ret_base58:" << ret_base58 << "\n";

        auto a_bese58 = dev::crypto::to_base58((char *) Address(addr).data(), 20);
        std::cout << "a_base58:" << a_bese58 << "\n";
        return new_addr + ret_base58 + a_bese58;
}



 std::string toNewAddress(std::string const& _addr){
     /**
      * private-key: 4N76qbS6YMA4KYbEwmW8CKLnVUtpfkegLPziaJ2zRQmi
        address    : efee5b4be9ba66b2f371d305cd4e0788fa2c7661
        public-key : fb27fe5ce0e83b5541562ed12e32d7e8aeb1fd1a9679267d5541800e4ecaab6d71cdd290205c7a3d945b83cab5af1c80ad9fa1d367b1e099f7dbfbce55c9b351
        3eb6689f1d9925ab60b92ff7614902a88302fd939bf9edb3c2b7c12ef41c75d4

        3eb6
        ret_base58:2byW5c
        a_base58:4LseofXfxvKS1ZBZzcVqH8cEGxyn
        brc2byW5c4LseofXfxvKS1ZBZzcVqH8cEGxyn
      *
      * **/
    using h192 = FixedHash<24>;
     Address addr = Address(_addr);
    auto hash = sha3(sha3(Address(addr)));
    //auto hash4= hash.ref().cropped(0,4);
    auto hashBytes = sha3(sha3(Address(addr))).ref().cropped(0,4).toBytes();
    std::cout << hash <<std::endl;
    std::cout << hashBytes <<std::endl;
    std::cout <<toHex(hashBytes)<<"  "<< toHex(addr)<<std::endl;
    auto newAddress = toHex(hashBytes) + toHex(addr);
    h192 newAddress_ = h192(toHex(hashBytes) + toHex(addr));
    std::cout << newAddress <<std::endl;
    return "brc" + dev::crypto::to_base58((char *) newAddress_.data(), 24);
 }
 bool  checkNewAddress(std::string const& _addr){
    // check brc
    try {
        if (_addr.substr(0, 3) != "brc") {
            std::cout << "check brc is false";
            return false;
        }
        auto base58Str = _addr.substr(3, _addr.length());
        std::cout << base58Str << std::endl;
        auto hash_addr = dev::crypto::from_base58(base58Str);
        std::cout << hash_addr << std::endl;
        std::cout << hash_addr.size() << std::endl;
        if (hash_addr.size() != 24) {
            std::cout << "error! the address is Error:" << std::endl;
            return false;
        }
        auto hash_addr_str = toHex(hash_addr);
        ///check addr hash
        auto hash = hash_addr_str.substr(0, 8);
        auto old_addr = hash_addr_str.substr(8, 48);
        auto hash4 = toHex(sha3(sha3(Address(old_addr))).ref().cropped(0, 4).toBytes());
        if (hash != hash4) {
            std::cout << "error! the address is Error:" << hash4 << std::endl;
            return false;
        }
        std::cout << hash_addr_str.substr(0, 8) << "  " << hash_addr_str.substr(8, 48) << std::endl;
    }
    catch (dev::crypto::Base58_decode_exception& e){
        std::cout<< e.what() <<std::endl;
        return false;
    }
    catch (std::exception &e){
        std::cout<< e.what() <<std::endl;
        return false;
    }
    catch (...){
        std::cout<< "eeeeeeee" <<std::endl;
        return false;
    }
     return true;
 }


int main(int argc, char *argv[]){
    std:: cout << "*************************************" << std::endl;
    auto ret__ = jsToNewAddress(Address("efee5b4be9ba66b2f371d305cd4e0788fa2c7661"));
    std::cout << ret__ << std::endl;
    std::cout << jsToAddressFromNewAddress(ret__) << std::endl;
    std:: cout << "*************************************" << std::endl;
    auto test_a =jsToNewAddress(Address("0x0000000067656e6573697343616e646964617465"));
    std::cout << test_a<<std::endl;
    std::cout << jsToAddressFromNewAddress(test_a)<<std::endl;

    try {
        std::string a1= "0x4d51ba213bad0f7ed97d336b2c00dc76576aac98";
        std::string a2= "4d51ba213bad0f7ed97d336b2c00dc76576aac98";
        std::string a3= "brcDWqUiYGMfSwUGJ5comAWFa7WFsiKofPTS";

        std::string a4=    "4d51ba213bad0f7ed97d336b2c00dcaac98";
        std::string a41= "0x4d51ba213bad0f7ed97d336b2c00dcaac98";
        std::string a5= "4d51ba213bad0f7ed97d336b2c00dc76576aac985";
        std::string a6= "brcDWqUiYGMfSwUGJ5comAWFa7WFsiKofWTS";
        std::string a7= "";
        std::string a8= "0x";

        std::cout << "****************************************************"<<std::endl;
        std::cout << "a1:" << jsToAddressAcceptAllAddress(a1)<<std::endl;
        std::cout << "a2:" << jsToAddressAcceptAllAddress(a2)<<std::endl;
        std::cout << "a3:" << jsToAddressAcceptAllAddress(a3)<<std::endl;
        //std::cout << "a41:" << jsToAddressAcceptAllAddress(a41)<<std::endl;
        //std::cout << "a4:" << jsToAddressAcceptAllAddress(a4)<<std::endl;
        //std::cout << "a5:" << jsToAddressAcceptAllAddress(a5)<<std::endl;
        //std::cout << "a6:" << jsToAddressAcceptAllAddress(a6)<<std::endl;
        std::cout << "a7:" << jsToAddressAcceptAllAddress(a7)<<std::endl;
        //std::cout << "a8:" << jsToAddressAcceptAllAddress(a8)<<std::endl;

    }catch (...){
        std::cout << "address Error";
    }


//    for(int i=0; i<10000; i++){
//        auto key_pair = KeyPair::create();
//        auto sec = key_pair.secret();
//        auto oldAddr = toString(key_pair.address());
//        auto ret = jsToNewAddress(oldAddr);
//        std::cout << ret << "  ";
//        auto ret1 = jsToAddressFromNewAddress(ret);
//        std::cout << ret1 ;
//        if(Address(oldAddr) == ret1)
//            std::cout << "     ok "<<std::endl;
//        else
//        {
//            std::cout << "&&&&&&&&&&&&&&&&&";
//            return 0;
//        }
//    }

    return 0;
}