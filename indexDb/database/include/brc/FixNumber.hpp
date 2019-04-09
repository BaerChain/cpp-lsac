#pragma once


#include <chainbase/chainbase.hpp>
#include <libdevcore/FixedHash.h>

using namespace dev;

namespace brc{
    namespace db{

        typedef std::vector<uint8_t, chainbase::allocator<uint8_t> >  shared_hash256;
        typedef std::vector<uint8_t, chainbase::allocator<uint8_t> >  shared_Address;

        template <typename S, typename  T>
        inline  void shared_to_fix_number(const S &source, T &to){
            for(auto i = 0; i < to.size ; i++){
                to[i] = source[i];
            }
        }

        template <typename S, typename  T>
        inline void fix_number_to_shared(const S &source, T &to){
            for(auto i = 0; i < source.size ; i++){
                to.push_back(source[i]);
            }
        }


        struct shared_compare
        {
            bool operator () (const shared_hash256 &t1, const shared_hash256 &t2) const {
                if(t1.size() == 32){
                    FixedHash<32> to1;
                    FixedHash<32> to2;
                    shared_to_fix_number(t1, to1);
                    shared_to_fix_number(t2, to2);
                    return this->operator()(to1, to2);
                }
                else if(t1.size() == 20){
                    FixedHash<20> to1;
                    FixedHash<20> to2;
                    shared_to_fix_number(t1, to1);
                    shared_to_fix_number(t2, to2);
                    return this->operator()(to1, to2);
                }
                //TODO
                std::cout << "error , please complete shared_compare.";
                exit(1);
                return false;
            }

            template <unsigned T>
            bool operator () (const FixedHash<T> &t1, const FixedHash<T> &t2) const {
                return t1 > t2;
            }


        };


        struct shared_number_256{

            uint32_t  ret[5];
        };






    }
}