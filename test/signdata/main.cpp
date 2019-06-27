#include <libdevcore/Common.h>
#include <libdevcrypto/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/vector_ref.h>
#include <libdevcrypto/base58.h>
#include <libdevcore/SHA3.h>


using namespace dev;


int main(int argc, char *argv[])
{
    KeyPair _key = KeyPair::create();
    std::cout << "address: " << toHex(_key.address()) << std::endl;
    std::cout << "public: " << _key.pub() << std::endl;
    std::cout << "private_key: " << dev::crypto::to_base58((char*)_key.secret().data(),32) << std::endl;


    std::string _from = toHex(_key.address()); 
    std::string _to = "0xe523e7c59a0725afd08bc9751c89eed6f8e16dec";
    unsigned _blockNum = 20;

    RLPStream _rlp(3);
    _rlp << Address(_from) << Address(_to) << _blockNum;
    Signature _sign = sign(_key.secret(), sha3(_rlp.out()));

    std::cout << "sign: " << _sign << std::endl;

    if(verify(_key.pub(), _sign, sha3(_rlp.out())))
    {
        std::cout << "_sign success!" << std::endl;
    }else{
        std::cout << " _sign error!" << std::endl;
        return 0;
    }

    std::cout << " recover:" << recover(_sign, sha3(_rlp.out())) << std::endl;
    
    auto _r = recover(_sign, sha3(_rlp.out()));

    std::cout << " right Address:" << right160(dev::sha3(bytesConstRef(_r.data(),sizeof(_r)))) << std::endl;

}