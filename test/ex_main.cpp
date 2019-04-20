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
#include <libbrcdchain/Transaction.h>
#include <libdevcrypto/base58.h>
#include <libdevcore/Common.h>


#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>

namespace dr = dev::brc;
namespace dre  = dev::brc::ex;
namespace dbt = dev::brc::transationTool;
using boost::asio::ip::tcp;
using std::string;
using namespace dev;

namespace testex {

    struct account {
        dev::Address address;
        dev::u256 brc_balance;
        dev::u256 fbrc_balance;
        dev::u256 balance;
        dev::u256 fbalance;
        u256 nonce;


        std::string tojs() const {
            Json::Value vv;
            vv["address"] = toJS(address);
            vv["brc_balance"] = toJS(brc_balance);
            vv["fbrc_balance"] = toJS(fbrc_balance);
            vv["balance"] = toJS(balance);
            vv["fbalance"] = toJS(fbalance);
            vv["nonce"] = toJS(nonce);


            return vv.toStyledString();
        }


        bool operator==(const account &ac) {
            cwarn << this->tojs();
            cwarn << ac.tojs();
            return true;
        }

        account operator%(const account &ac) {


            cwarn << "address:   " << toJS(this->address);
            std::string f1 = std::max(ac.brc_balance, brc_balance) == ac.brc_balance ? "+" : "-";
            std::string f2 = std::max(ac.fbrc_balance, fbrc_balance) == ac.fbrc_balance ? "+" : "-";
            std::string f3 = std::max(ac.balance, balance) == ac.balance ? "+" : "-";
            std::string f4 = std::max(ac.fbalance, fbalance) == ac.fbalance ? "+" : "-";

            cwarn << "brc _balance: " << f1 << diff(ac.brc_balance, brc_balance);
            cwarn << "fbrc_balance: " << f2 << diff(ac.fbrc_balance, fbrc_balance);
            cwarn << "     balance: " << f3 << diff(ac.balance, balance);
            cwarn << "    fbalance: " << f4 << diff(ac.fbalance, fbalance);
            return *this;
        }
    };

    Json::Value from_rpc(const std::string &str) {
        auto readerBuilder = Json::CharReaderBuilder();
        auto sreader = readerBuilder.newCharReader();
        Json::Value root;
        std::string errotStr;
        if (!sreader->parse(str.c_str(), str.c_str() + str.length(), &root, &errotStr)) {
            cwarn << "error json rpc: " << str;
        }
        return root["result"];
    }

    template<typename T>
    void from_json(const std::string &source, T &t);

    template<>
    void from_json<account>(const std::string &source, account &ac) {
        auto js = from_rpc(source);
        ac.address = dev::Address(js["Address"].asString());
        ac.brc_balance = u256(fromBigEndian<u256>(fromHex(js["BRC"].asString())));
        ac.fbrc_balance = u256(fromBigEndian<u256>(fromHex(js["FBRC"].asString())));
        ac.fbalance = u256(fromBigEndian<u256>(fromHex(js["FBalance"].asString())));
        ac.balance = u256(fromBigEndian<u256>(fromHex(js["balance"].asString())));
        ac.nonce = u256(fromBigEndian<u256>(fromHex(js["nonce"].asString())));
    }


    bytes packed_operation_data(const dbt::pendingorder_opearaion &p) {
        RLPStream rlp;
        std::vector<bytes> _v;
        // rlp.append(p->serialize());
        _v.push_back(p.serialize());

        rlp.appendVector<bytes>(_v);
        return rlp.out();
    }


    class test_helper {
    public:
        test_helper(const std::string &url) : url_ip(url) {}

        void packed_transaction(dre::order_type type, dre::order_token_type token_type, dre::order_buy_type buy_type,
                                dev::u256 price, dev::u256 amount) {

        }


        void packed_transaction(const dbt::pendingorder_opearaion &op) {
            auto account = get_address_info(op.m_from);
//            cwarn << "account.nonce " << account.nonce << " from " << op.m_from;
            brc::TransactionSkeleton ts;
            ts.creation = false;
            ts.from = op.m_from;
            ts.to = Address("00000000000000000000000000000000766f7465");
            ts.value = 0x1388;
            ts.data = packed_operation_data(op);
            ts.nonce = account.nonce;
            ts.gas = u256(0x6000);
            ts.gasPrice = u256(0x1388);
            brc::Transaction sign_t(ts, keys[op.m_from]);
//            cwarn << "send trx " << dev::brc::toJson(sign_t);

            send_raw_transaction(sign_t);
        }

        dbt::pendingorder_opearaion up_down_op(const dbt::pendingorder_opearaion &op, const Address &ad) {
            dbt::pendingorder_opearaion pp(op);
            pp.m_from = ad;
            pp.m_Pendingorder_Token_type = pp.m_Pendingorder_Token_type ^ 1;
            pp.m_Pendingorder_type = op.m_Pendingorder_type == 1 ? 2 : 1;

            return pp;
        }

        account get_address_info(const dev::Address &ad) {
            std::string rpc = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_getBalance\",\"params\":[\"" + dev::toJS(ad) +
                              "\", \"-1\"],\"id\":1}";
            std::string ret;
            http_post(rpc, ret);
            account ac;
            if (ret.size() != 0) {
                from_json(ret, ac);
            }
            return ac;
        }


        void send_raw_transaction(const brc::Transaction &tx) {
            auto s = toHexPrefixed(tx.rlp());
            std::string rpc =
                    "{\"jsonrpc\":\"2.0\",\"method\":\"brc_sendRawTransaction\",\"params\":[\"" + s + "\"],\"id\":1}";
            cwarn << "send trx " << toJS(tx.from()) << "   tx: " << tx.sha3() << " : " << rpc;
            std::string ret;
            http_post(rpc, ret);
            cwarn << "push transaction ret: " << ret << std::endl;
        }


        int http_post(const std::string &send_msg, std::string &reponse_data) {
            jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(url_ip);
            _httpClient.SetTimeout(20 * 1000);
            _httpClient.SendRPCMessage(send_msg, reponse_data);


//            std::string host = "127.0.0.1";
//            std::string port = "8081";
//            std::string page = "/";
//            try
//            {
//                boost::asio::io_service io_service;
//                if(io_service.stopped())
//                    io_service.reset();
//
//                tcp::resolver resolver(io_service);
//                tcp::resolver::query query(host, port);
//                tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
//
//                tcp::socket socket(io_service);
//                boost::asio::connect(socket, endpoint_iterator);
//
//                // Form the request. We specify the "Connection: close" header so that the
//                // server will close the socket after transmitting the response. This will
//                // allow us to treat all data up until the EOF as the content.
//                boost::asio::streambuf request;
//                std::ostream request_stream(&request);
//                request_stream << "POST " << page << " HTTP/1.0\r\n";
//                request_stream << "Host: " << host << ":" << port << "\r\n";
//                request_stream << "Accept: */*\r\n";
//                request_stream << "Content-Length: " << send_msg.length() << "\r\n";
//                request_stream << "Content-Type: application/x-www-form-urlencoded\r\n";
//                request_stream << "Connection: close\r\n\r\n";
//                request_stream << send_msg;
//
//                // Send the request.
//                boost::asio::write(socket, request);
//
//                // Read the response status line. The response streambuf will automatically
//                // grow to accommodate the entire line. The growth may be limited by passing
//                // a maximum size to the streambuf constructor.
//                boost::asio::streambuf response;
//                boost::asio::read_until(socket, response, "\r\n");
//
//                // Check that response is OK.
//                std::istream response_stream(&response);
//                std::string http_version;
//                response_stream >> http_version;
//                unsigned int status_code;
//                response_stream >> status_code;
//                std::string status_message;
//                std::getline(response_stream, status_message);
//                if (!response_stream || http_version.substr(0, 5) != "HTTP/")
//                {
//                    reponse_data = "Invalid response";
//                    return -2;
//                }
//                if (status_code != 200)
//                {
//                    reponse_data = "Response returned with status code != 200 " ;
//                    return status_code;
//                }
//
//                std::string header;
//                std::vector<string> headers;
//                while (std::getline(response_stream, header) && header != "\r")
//                    headers.push_back(header);
//
//                boost::system::error_code error;
//                while (boost::asio::read(socket, response,
//                                         boost::asio::transfer_at_least(1), error))
//                {
//                }
//
//                if (response.size())
//                {
//                    std::istream response_stream(&response);
//                    std::istreambuf_iterator<char> eos;
//                    reponse_data = string(std::istreambuf_iterator<char>(response_stream), eos);
//                }
//
//                if (error != boost::asio::error::eof)
//                {
//                    reponse_data = error.message();
//                    return -3;
//                }
//            }
//            catch(std::exception& e)
//            {
//                reponse_data = e.what();
//                return -4;
//            }
            return 0;
        }

        void set_keys() {
            std::vector<std::string> obj{
                    "8RioSGhgNUKFZopC2rR3HRDD78Sc48gci4pkVhsduZve",
                    "BeBU5mQE5nsymzvs86NGUAqZBwNbV4WQH45WBj9VNTDS",
                    "D78xbqv1YDHJELbV2AqUsckDK4yCctprdcLZ3mrJEJxJ",
                    "5dRiAXBNgtZdsgY63awF9hSq863ecxMLDDF6hbzJfgAd",
                    "HYem7Su52cPGHviFX6sjxtvLUnREuQ1tEyd8yF35pNUS",
                    "A3rhL66RTF4xdS7ppHe396BGb7zvJASm5H5NzaiXubTM"
            };
            cwarn << "init address , keys";
            for (auto &key : obj) {
                auto keyPair = dev::KeyPair(dev::Secret(dev::crypto::from_base58(key)));
                keys[keyPair.address()] = keyPair.secret();
                cwarn << toJS(keyPair.address()) << " : " << key;

            }
        }

        std::string url_ip;
        std::map<Address, Secret> keys;
    };

}

/*
 *
            enum order_type : uint8_t {
                null_type = 0,
                sell = 1,
                buy = 2
            };

            enum order_token_type : uint8_t {
                BRC = 0,
                FUEL                   // => cook
            };


            enum order_buy_type : uint8_t {
                all_price = 0,
                only_price
            };

0xe523e7c59a0725afd08bc9751c89eed6f8e16dec : 8RioSGhgNUKFZopC2rR3HRDD78Sc48gci4pkVhsduZve
0xb0de975d99fa9a3f94946fb9ee8ac7a166a5a856 : BeBU5mQE5nsymzvs86NGUAqZBwNbV4WQH45WBj9VNTDS
0x2e7abb8dc2ef5743d66bf83bca574008dd2c00ad : D78xbqv1YDHJELbV2AqUsckDK4yCctprdcLZ3mrJEJxJ
0x6aeef4abd6eb8e13ae5ab009e2d45fca9ed18f77 : 5dRiAXBNgtZdsgY63awF9hSq863ecxMLDDF6hbzJfgAd
0xe53768c8b089cf35adb8c85fa183f36d73616a53 : HYem7Su52cPGHviFX6sjxtvLUnREuQ1tEyd8yF35pNUS
0xaa800d077c0afe7732b66ac58e9120f446af14a0 : A3rhL66RTF4xdS7ppHe396BGb7zvJASm5H5NzaiXubTM


 *
 * */

void test1() {
    testex::test_helper helper("127.0.0.1:8081");
    helper.set_keys();


    uint8_t token_type[] = {0, 1};
    uint8_t type[] = {1, 2};
    {
        for (uint8_t i = 0; i < 2; i++) {
            for (uint8_t j = 0; j < 2; j++) {
                uint8_t send_type = type[i];                  //buy
                uint8_t send_token_type = token_type[j];            //BRC
                uint8_t send_buy_type = 1;
                auto ad1 = Address("0xb0de975d99fa9a3f94946fb9ee8ac7a166a5a856");
                auto ad2 = Address("0x2e7abb8dc2ef5743d66bf83bca574008dd2c00ad");
                /*-----------------------------------------*/
                dbt::pendingorder_opearaion op1;
                op1.m_type = 3;
                op1.m_from = ad1;
                op1.m_Pendingorder_type = send_type;
                op1.m_Pendingorder_Token_type = send_token_type;
                op1.m_Pendingorder_buy_type = send_buy_type;
                op1.m_Pendingorder_num = 2;
                op1.m_Pendingorder_price = 2;

                dbt::pendingorder_opearaion op2 = helper.up_down_op(op1, ad2);

                auto s_ac1 = helper.get_address_info(ad1);
                auto s_ac2 = helper.get_address_info(ad2);

                helper.packed_transaction(op1);
                sleep(2);
                helper.packed_transaction(op2);

                auto ac1 = helper.get_address_info(ad1);
                auto ac2 = helper.get_address_info(ad2);
                s_ac1 % ac1;
                s_ac2 % ac2;
                sleep(2);
            }
        }
    }
}


void test2() {
    testex::test_helper helper("127.0.0.1:8081");
    helper.set_keys();

    std::vector<Address> senders = {Address("0xe523e7c59a0725afd08bc9751c89eed6f8e16dec"),
                                    Address("0xb0de975d99fa9a3f94946fb9ee8ac7a166a5a856"),
                                    Address("0x2e7abb8dc2ef5743d66bf83bca574008dd2c00ad"),
                                    Address("0x6aeef4abd6eb8e13ae5ab009e2d45fca9ed18f77"),
                                    Address("0xe53768c8b089cf35adb8c85fa183f36d73616a53")
    };
    std::vector<testex::account> pre_accounts;


    uint8_t type[] = {1, 2};
    uint8_t token_type[] = {0, 1};

    for (uint8_t i = 0; i < 2; i++) {
        for (uint8_t j = 0; j < 2; j++) {

            uint8_t send_type = type[i];                  //buy
            uint8_t send_token_type = token_type[j];            //BRC
            uint8_t send_buy_type = 1;

            for (uint32_t k = 0; k < senders.size(); k++) {

                auto ad1 = senders[k];
                pre_accounts.push_back(helper.get_address_info(ad1));
                dbt::pendingorder_opearaion op1;
                op1.m_type = 3;
                op1.m_from = ad1;
                op1.m_Pendingorder_type = send_type;
                op1.m_Pendingorder_Token_type = send_token_type;
                op1.m_Pendingorder_buy_type = send_buy_type;
                op1.m_Pendingorder_num = k + 1;
                op1.m_Pendingorder_price = k + 1;
//                if(i == 1 && j == 0 && k == 3){
//                    helper.packed_transaction(op1);
//                }
                helper.packed_transaction(op1);
                sleep(2);

            }

            //
            Address from("0xaa800d077c0afe7732b66ac58e9120f446af14a0");
            auto pre_from = helper.get_address_info(from);

            dbt::pendingorder_opearaion op2;
            op2.m_type = 3;
            op2.m_from = from;
            op2.m_Pendingorder_type = send_type;
            op2.m_Pendingorder_Token_type = send_token_type;
            op2.m_Pendingorder_buy_type = send_buy_type;
            op2.m_Pendingorder_num = 5;
            op2.m_Pendingorder_price = 3;

            op2 = helper.up_down_op(op2, from);

            helper.packed_transaction(op2);

            sleep(1);
            for (uint32_t i = 0; i < senders.size(); i++) {
                auto ad1 = pre_accounts[i].address;
                auto ac = helper.get_address_info(ad1);
                pre_accounts[i] % ac;
            }

            pre_from % helper.get_address_info(from);

            pre_accounts.clear();
            cwarn << "-------------------------- clear --------------------------";
            sleep(2);
        }
    }


}


void test3() {

    testex::test_helper helper("127.0.0.1:8081");
    helper.set_keys();


    uint8_t send_type = 1;                  //buy
    uint8_t send_token_type = 0;            //BRC
    uint8_t send_buy_type = 1;


    auto ad1 = Address("0xb0de975d99fa9a3f94946fb9ee8ac7a166a5a856");
    auto ad2 = Address("0x2e7abb8dc2ef5743d66bf83bca574008dd2c00ad");
    /*-----------------------------------------*/
    dbt::pendingorder_opearaion op1;
    op1.m_type = 3;
    op1.m_from = ad1;
    op1.m_Pendingorder_type = 2;
    op1.m_Pendingorder_Token_type = send_token_type ^ 1;
    op1.m_Pendingorder_buy_type = send_buy_type;
    op1.m_Pendingorder_num = 1;
    op1.m_Pendingorder_price = 3;

    dbt::pendingorder_opearaion op3;
    op3.m_type = 3;
    op3.m_from = ad1;
    op3.m_Pendingorder_type = 2;
    op3.m_Pendingorder_Token_type = send_token_type ^ 1;
    op3.m_Pendingorder_buy_type = send_buy_type;
    op3.m_Pendingorder_num = 4;
    op3.m_Pendingorder_price = 4;


    helper.packed_transaction(op1);
    sleep(1);
    helper.packed_transaction(op3);
    sleep(1);


    dbt::pendingorder_opearaion op2;
    op2.m_type = 3;
    op2.m_from = ad2;
    op2.m_Pendingorder_type = send_type;
    op2.m_Pendingorder_Token_type = send_token_type;
    op2.m_Pendingorder_buy_type = send_buy_type;
    op2.m_Pendingorder_num = 4;
    op2.m_Pendingorder_price = 4;

    helper.packed_transaction(op2);


}


void test_self() {
    testex::test_helper helper("127.0.0.1:8081");
    helper.set_keys();


    auto ad1 = Address("0xb0de975d99fa9a3f94946fb9ee8ac7a166a5a856");
    auto ad2 = Address("0x2e7abb8dc2ef5743d66bf83bca574008dd2c00ad");
    /*-----------------------------------------*/
    dbt::pendingorder_opearaion op1;
    op1.m_type = 3;
    op1.m_from = ad1;
    op1.m_Pendingorder_type = 2;
    op1.m_Pendingorder_Token_type = 1;
    op1.m_Pendingorder_buy_type = 1;
    op1.m_Pendingorder_num = 3;
    op1.m_Pendingorder_price = 3;


    helper.packed_transaction(op1);

//    sleep(1);
    dbt::pendingorder_opearaion op2;
    op2.m_type = 3;
    op2.m_from = ad2;
    op2.m_Pendingorder_type = 1;
    op2.m_Pendingorder_Token_type = 0;
    op2.m_Pendingorder_buy_type = 1;
    op2.m_Pendingorder_num = 3;
    op2.m_Pendingorder_price = 3;

    helper.packed_transaction(op2);


}

void test_one() {
    testex::test_helper helper("127.0.0.1:8081");
    helper.set_keys();


    auto ad1 = Address("0xb0de975d99fa9a3f94946fb9ee8ac7a166a5a856");
//    auto ad2 = Address("0x2e7abb8dc2ef5743d66bf83bca574008dd2c00ad");
    /*-----------------------------------------*/
    dbt::pendingorder_opearaion op1;
    op1.m_type = 3;
    op1.m_from = ad1;
    op1.m_Pendingorder_type = 2;
    op1.m_Pendingorder_Token_type = 1;
    op1.m_Pendingorder_buy_type = 1;
    op1.m_Pendingorder_num = 3;
    op1.m_Pendingorder_price = 3;
    helper.packed_transaction(op1);



}


int main(int argc, char **argv) {

    int c = (int) (**(argv + 1)) - 48;

    switch (c) {
        case 1:
            test_one();
            break;
        case 2:
            test2();
            break;
        case 3:
            test3();
            break;
        case 4:
            test_self();
            break;
        case 5:
            test1();
            break;
    }

    return 0;
}