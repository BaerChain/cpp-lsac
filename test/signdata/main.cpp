#include <libdevcore/Common.h>
#include <libdevcrypto/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/vector_ref.h>
#include <libdevcrypto/base58.h>
#include <libdevcore/SHA3.h>


using namespace dev;
using namespace std;


vector<tuple<string, string, unsigned>> _needSignData = {
        {"0xe523e7c59a0725afd08bc9751c89eed6f8e16dec", "0x042610e447c94ff0824b7aa89fab123930edc805", 1},
        {"0xe523e7c59a0725afd08bc9751c89eed6f8e16dec", "0x042610e447c94ff0824b7aa89fab123930edc805", 100}
};




map<string, string> _otherAccount = {
        {"0x82564e0146bc6b0ba98835aee58e54d484e8c81d", "3ExgDCQFhexa8AftFP35LaMEqvUTcpQeB4nW4TCBSe5e"},
        {"0xef2347f270008473897df5059b914fd078ef3fac", "BaHzrQMcH1ARz3zx8s1d7D9BmFxFGwtepXgMyjEWateU"}
};

pair<string, string> _signAccount = {"0xe523e7c59a0725afd08bc9751c89eed6f8e16dec", "8RioSGhgNUKFZopC2rR3HRDD78Sc48gci4pkVhsduZve"};




int main(int argc, char *argv[])
{
    for(auto i : _needSignData)
    {
         RLPStream _rlp(3);
         _rlp << Address(std::get<0>(i)) << Address(std::get<1>(i)) << std::get<2>(i);
         auto _keypair = KeyPair(Secret(dev::crypto::from_base58(_signAccount.second)));
         Signature  _sign = sign(_keypair.secret(), sha3(_rlp.out()));
         cout << " _signAccount data: " << _sign << endl;


         for(auto j : _otherAccount)
         {
                RLPStream _otherAccountrlp(4);
                _otherAccountrlp << Address(std::get<0>(i)) << Address(std::get<1>(i)) << std::get<2>(i) << _sign;
                auto _otherAccountKey = KeyPair(Secret(dev::crypto::from_base58(j.second)));
                Signature  _otherAccountSign = sign(_otherAccountKey.secret(), sha3(_otherAccountrlp.out()));
                cout << "_otherAccountSign :" << _otherAccountSign << endl;
         }
    }
}




//int main(int argc, char *argv[])
//{
//    KeyPair _key = KeyPair::create();
//    std::cout << "address: " << toHex(_key.address()) << std::endl;
//    std::cout << "public: " << _key.pub() << std::endl;
//    std::cout << "private_key: " << dev::crypto::to_base58((char*)_key.secret().data(),32) << std::endl;
//
//
//    std::string _from = toHex(_key.address());
//    std::string _to = "0xe523e7c59a0725afd08bc9751c89eed6f8e16dec";
//    unsigned _blockNum = 20;
//
//    RLPStream _rlp(3);
//    _rlp << Address(_from) << Address(_to) << _blockNum;
//    Signature _sign = sign(_key.secret(), sha3(_rlp.out()));
//
//    std::cout << "sign: " << _sign << std::endl;
//
//    if(verify(_key.pub(), _sign, sha3(_rlp.out())))
//    {
//        std::cout << "_sign success!" << std::endl;
//    }else{
//        std::cout << " _sign error!" << std::endl;
//        return 0;
//    }
//
//    std::cout << " recover:" << recover(_sign, sha3(_rlp.out())) << std::endl;
//
//    auto _r = recover(_sign, sha3(_rlp.out()));
//
//    std::cout << " right Address:" << right160(dev::sha3(bytesConstRef(_r.data(),sizeof(_r)))) << std::endl;
//
//}

