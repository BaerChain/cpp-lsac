#include <libdevcrypto/Common.h>
#include <libdevcore/Common.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/Base64.h>
#include <libethereum/Transaction.h>
#include <libdevcore/CommonJS.h>
#include <libweb3jsonrpc/JsonHelper.h>
#include <libdevcrypto/base58.h>
#include <string>
dev::bytesConstRef to_byteRef(const std::string &source){
    dev::bytesConstRef ff(source);
    return ff;
}


using namespace dev;
int main(int argc, char *argv[]){
    std::string pwd = "DdQEYux64qDRt7Ey9RkEc6TWxcskgpm7dNeRv27EVsmh5oJALX5DRAguo4mZ41DaE5jNXZkhnAwuWUXKYwUsD9Q";
    auto seed = to_byteRef(pwd);
    auto ss = dev::sha3(seed);

    dev::Secret secret(ss);
    dev::KeyPair kp(secret);// = dev::KeyPair::fromEncryptedSeed(pwd, pwd);
    dev::Public  pubkk = kp.pub();
    std::cout << "address:" << kp.address() << std::endl;
//    std::cout << "public: "<< kp.pub() << std::endl;
    auto vv_size = pubkk.asBytes();
    std::cout << "vv " << vv_size.size() << std::endl;
    std::cout << "private encode: " <<  dev::crypto::to_base58((char*)secret.data(), 32) << std::endl;
    std::cout << "private: " <<  secret.ref().toString() << std::endl;
    std::cout << "public: " <<  dev::crypto::to_base58((char*)vv_size.data(), vv_size.size()) << std::endl;

    std::cout << "-------------------------------------------------------------------------------------------------------------------------------\n";

    std::string wif = "b236442ef143b6883a90a9d6cec0951dcc41fb84413355b67567304e6f45d122b42c0e5f30beeffd609a448526d337506f865a61343aa9a8dbc6de725c026581";
//    auto decode = dev::crypto::from_base58(wif);
//    std::cout << "decode size : " << decode.size() << std::endl;
//    for(auto &c : decode){
//        std::cout << c ;
//    }
//    auto private_key = Secret(decode);
    auto public_key = Public(wif);
    std::cout << "public_key " << dev::crypto::to_base58((char*)public_key.data(), 64) << std::endl;
    if(public_key == pubkk){
        std::cout << "private key equal." << std::endl;
    }





    std::cout << "-------------------------------------------------------------------------------------------------------------------------------\n";


    auto sign = dev::sign(secret, ss);
    std::cout << "sing: " << sign << std::endl;
    auto recover_public = dev::recover(sign, ss);
    std::cout << "recover_public:" << recover_public << std::endl;
    if(recover_public == kp.pub()){
        std::cout << "check sign success." << std::endl;
    }
//Transaction(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, Address const& _dest, bytes const& _data, u256 const& _nonce = Invalid256):
//		TransactionBase(_value, _gasPrice, _gas, _dest, _data, _nonce)
    dev::eth::Transaction trx(u256(1), u256(0x4a817c800), u256(1000000), kp.address(), bytes(), u256(1));

    trx.sign(secret);

    std::cout << dev::toJS(trx.rlp()) << std::endl;
    std::cout << eth::toJson(trx) << std::endl;
//    std::cout << dev::toJson(Transaction(jsToBytes(trx.rlp(), OnFailed::Throw), dev::eth::CheckTransaction::Everything)) << std::endl;


    return 0;
}




//#define BOOST_TEST_MODULE pub_key_test
//#define BOOST_TEST_DYN_LINK
//#include <boost/test/unit_test.hpp>
//
//
//BOOST_AUTO_TEST_SUITE(pub_key_test)
//
//    BOOST_AUTO_TEST_CASE(pub_key_test) {
//
//
//
//    }
//
//
//
//BOOST_AUTO_TEST_SUITE_END()
//


