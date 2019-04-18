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
namespace dr = dev::brc;
namespace dre  = dev::brc::ex;
namespace dbt = dev::brc::transationTool;

using namespace dev;

namespace testex {

    struct account {
        dev::Address address;
        dev::u256 brc_balance;
        dev::u256 fbrc_balance;
        dev::u256 balance;
        dev::u256 fbalance;
        u256 nonce;


        std::string tojs() const{
            Json::Value vv;
            vv["address"] = toJS(address);
            vv["brc_balance"] = toJS(brc_balance);
            vv["fbrc_balance"] = toJS(fbrc_balance);
            vv["balance"] = toJS(balance);
            vv["fbalance"] = toJS(fbalance);
            vv["nonce"] = toJS(nonce);


            return vv.toStyledString();
        }


        bool operator == (const account &ac){
            cwarn <<  this->tojs();
            cwarn <<  ac.tojs();
            return true;
        }

        account operator % (const account &ac){

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
        ac.balance = u256(fromBigEndian<u256>(fromHex(js["balance"].asString())));
        ac.fbalance = u256(fromBigEndian<u256>(fromHex(js["fbalance"].asString())));
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
            cwarn << "account.nonce " << account.nonce << " from " << op.m_from;
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
            cwarn << "send trx " << toJS(sign_t);

            send_raw_transaction(sign_t);
        }

        dbt::pendingorder_opearaion up_down_op(const dbt::pendingorder_opearaion &op, const Address &ad){
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


        void send_raw_transaction(const brc::Transaction &tx){
            auto s = toHexPrefixed(tx.rlp());
            std::string rpc = "{\"jsonrpc\":\"2.0\",\"method\":\"brc_sendRawTransaction\",\"params\":[\"" + s +"\"],\"id\":1}";
            std::string ret;
            http_post(rpc, ret);
            cwarn << "push transaction ret: " << ret << std::endl;
        }


        void http_post(const std::string &send_msg, std::string &ret) {
            jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(url_ip);
            _httpClient.SendRPCMessage(send_msg, ret);
        }

        void set_keys(){
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
                cwarn << toJS(keyPair.address())  << " : " << key ;

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

int main(int argc, char **argv) {
    testex::test_helper helper("127.0.0.1:8081");
    helper.set_keys();


    uint8_t token_type[] = {0, 1};
    uint8_t type[] = {1, 2};
    {
//        for(uint8_t i = 0;i < 2; i++){
//            for(uint8_t j = 0; j < 2; j++){
                uint8_t send_type = 2;                  //buy
                uint8_t send_token_type = 1;            //BRC
                uint8_t send_buy_type = 1;
                auto ad1 =  Address("0xb0de975d99fa9a3f94946fb9ee8ac7a166a5a856");
                auto ad2 =  Address("0x2e7abb8dc2ef5743d66bf83bca574008dd2c00ad");
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
                sleep(1);
                helper.packed_transaction(op2);

                auto ac1 = helper.get_address_info(ad1);
                auto ac2 = helper.get_address_info(ad2);
                s_ac1 % ac1;
                s_ac2 % ac2;
                sleep(2);
//            }
//        }
    }



    return 0;
}