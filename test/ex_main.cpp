
#include <libdevcore/SHA3.h>
#include <libdevcore/Log.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <cstdlib>

#include <json_spirit/JsonSpiritHeaders.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <libbrccore/Common.h>
#include <libbrcdchain/Transaction.h>
#include <libdevcore/Address.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
#include <libdevcore/JsonUtils.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/base58.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <libweb3jsonrpc/JsonHelper.h>
#include <libbrccore/CommonJS.h>
#include <brc/types.hpp>
#include <libbrcdchain/GasPricer.h>

using namespace dev::brc;
using namespace dev;



struct  book{

    int index = 0;
    int a = 0;
    int b = 0;


};

struct index_book{

    bool operator () (const book &b1, const book &b2) const{
        if(b1.a == b2.a){
            return b1.b > b2.b;
        }
        return b1.a > b2.a;
    }
};


int main(){

//    auto compare = [](const book &b1, const book &b2) -> bool{
//        if(b1.a == b2.a){
//            return b1.b > b2.b;
//        }
//        return b1.a > b2.a;
//    };
//
//
//    book b1{1, 1, 1};
//    book b2{2, 1, 2};
//    book b3{3, 2, 2};
//    book b4{4, 2, 1};
//    book b5{5, 3, 3};
//    book b6{6,3, 2};
//    book b7{7, 4, 5};
//    book b8{8, 9, 0};
//    book b9{9, 9, 5};
//    book b10{10, 20, 2};
//    book b11{11, 20, 0};
//    book b12{12, 20, 10};
//
//    std::map<int, book, index_book> t1;
//    t1[b1.index] = b1;
//    t1[b2.index] = b2;
//    t1[b3.index] = b3;
//    t1[b4.index] = b4;
//    t1[b5.index] = b5;
//    t1[b6.index] = b6;
//    t1[b7.index] = b7;
//    t1[b8.index] = b8;
//    t1[b9.index] = b9;
//    t1[b10.index] = b10;
//    t1[b11.index] = b11;
//    t1[b12.index] = b12;
//
//    for(auto &itr : t1){
//        std::cout << itr.second.index << "  a: " << itr.second.a  << "  b: " << itr.second.b << std::endl;
//    }
//



    return 0;
}