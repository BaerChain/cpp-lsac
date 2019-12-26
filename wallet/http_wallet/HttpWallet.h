#pragma once

#include "HttpWalletFace.h"

namespace dev{
    namespace wallet{
        class HttpWallet :public HttpWalletFace{
        public:
            HttpWallet(std::string const& _sUrl):m_send_url(_sUrl){}

            virtual Json::Value sign_transaction(Json::Value const&_param) ;

            virtual Json::Value sign_transaction_send(Json::Value const&_param);

            virtual Json::Value new_address(std::string const &_param) ;

            inline virtual RPCModules implementedModules() const { return RPCModules{ RPCModule{ "wallet", "1.0"}}; }

        private:
            std::string m_send_url="";
        };
    }
}
