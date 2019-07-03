#pragma once

#include <vector>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/Exceptions.h>
//#include <boost/exception/exception.hpp>






namespace dev {

    namespace crypto {
        std::string to_base58(const char *d, size_t s);

        std::string to_base58(const std::vector<char> &data);

        std::vector<uint8_t> from_base58(const std::string &base58_str);

        size_t from_base58(const std::string &base58_str, char *out_data, size_t out_data_len);


        /// from wif key(std::string) to public key
        /// \param str_pub
        /// \return Public objecjt
        Public from_string_to_pubKey(const std::string &str_pub);

        /// from public key to wif key(std::string)
        /// \param pub public key
        /// \return std::string wif key
        std::string to_pubKey(const Public &pub);
    }


}


