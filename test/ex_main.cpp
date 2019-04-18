//
// Created by fri on 2019/4/18.
//


/*
 *  only test exchange system.
 *
 *
 *
 *
 * */
#include <brc/types.hpp>
#include <libdevcore/Address.h>
#include <libweb3jsonrpc/JsonHelper.h>
#include <libdevcore/CommonJS.h>
#include <libdevcore/Log.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
namespace dr = dev::brc;
namespace dre  = dev::brc::ex;

using namespace dev;

namespace testex{

    struct accocunt{
        dev::Address    address;
        dev::u256       brc_balance;
        dev::u256       cook_balance;

    };

    Json::Value from_rpc(const std::string &str){
        auto readerBuilder = Json::CharReaderBuilder();
        auto sreader = readerBuilder.newCharReader();
        Json::Value root;
        std::string errotStr;
        if (!sreader->parse(str.c_str(), str.c_str() + str.length(), &root, &errotStr))
        {
           cwarn << "error json rpc: " << str ;
        }
        return root["result"];
    }

    template<typename T>
    void from_json(const std::string &source, T &t);

    template <>
    void from_json<accocunt>(const std::string &source, accocunt &ac){
        auto js = from_rpc(source);
//        ac.address = js["Address"];
    }




    class test_helper{
    public:
        test_helper(const std::string &url):url_ip(url){}
        void packed_transaction(dre::order_type type, dre::order_token_type  token_type, dre::order_buy_type buy_type){

        }


        accocunt get_address_info(const dev::Address &ad){
            std::string rpc = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_getBalance\",\"params\":[\"" + dev::toJS(ad) +"\"],\"id\":1}";
            std::string ret;
            http_post(rpc, ret);
            accocunt ac;
            if(ret.size() != 0){

            }
            return ac;
        }


        void http_post(const std::string &send_msg, std::string &ret){
            jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(url_ip);
            _httpClient.SendRPCMessage(send_msg, ret);
        }


        std::string url_ip;
    };








}









int main(int argc, char **argv){
    return 0;
}