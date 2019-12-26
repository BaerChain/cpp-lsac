#pragma once

#include <libweb3jsonrpc/ModularServer.h>

namespace dev{
    using namespace jsonrpc;
namespace wallet{

class HttpWalletFace : public ServerInterface<HttpWalletFace> {
public:
    HttpWalletFace() {
        this->bindAndAddMethod(Procedure("sign_transaction", PARAMS_BY_NAME, JSON_STRING,
                                         "param", JSON_OBJECT, NULL), &HttpWalletFace::sign_transactionI);

        this->bindAndAddMethod(Procedure("sign_transaction_send", PARAMS_BY_NAME, JSON_STRING,
                                         "param", JSON_OBJECT, NULL), &HttpWalletFace::sign_transaction_sendI);

        this->bindAndAddMethod(Procedure("new_address", PARAMS_BY_POSITION, JSON_OBJECT,
                                         "param1", JSON_STRING, NULL), &HttpWalletFace::new_addressI);
    }

    inline virtual void sign_transactionI(const Json::Value &request, Json::Value &response) {
        (void) request;
        response = this->sign_transaction(request["param"]);
    };

    inline virtual void sign_transaction_sendI(const Json::Value &request, Json::Value &response) {
        (void) request;
        response = this->sign_transaction_send(request["param"]);
    };

    inline virtual void new_addressI(const Json::Value &request, Json::Value &response) {
        (void) request;
        response = this->new_address(request[0u].asString());
    };


    virtual Json::Value sign_transaction(Json::Value const&_param) = 0;

    virtual Json::Value sign_transaction_send(Json::Value const&_param) = 0;

    virtual Json::Value new_address(std::string const &_param) = 0;
};
}
}
