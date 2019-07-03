#ifndef JSONRPC_CPP_STUB_DEV_RPC_ADMINBRCFACE_H_
#define JSONRPC_CPP_STUB_DEV_RPC_ADMINBRCFACE_H_

#include "ModularServer.h"

namespace dev {
    namespace rpc {
        class AdminBrcFace : public ServerInterface<AdminBrcFace>
        {
            public:
                AdminBrcFace()
                {
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_blockQueueStatus", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_blockQueueStatusI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_setAskPrice", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_setAskPriceI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_setBidPrice", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_setBidPriceI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_setMining", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_BOOLEAN,"param2",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_setMiningI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_findBlock", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_findBlockI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_blockQueueFirstUnknown", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_blockQueueFirstUnknownI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_blockQueueRetryUnknown", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_blockQueueRetryUnknownI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_allAccounts", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_ARRAY, "param1",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_allAccountsI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_newAccount", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_OBJECT,"param2",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_newAccountI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_setMiningBenefactor", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_setMiningBenefactorI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_inspect", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_inspectI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_reprocess", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_reprocessI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_vmTrace", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_INTEGER,"param3",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_vmTraceI);
                    this->bindAndAddMethod(jsonrpc::Procedure("admin_brc_getReceiptByHashAndIndex", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_INTEGER,"param3",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::admin_brc_getReceiptByHashAndIndexI);
                    this->bindAndAddMethod(jsonrpc::Procedure("miner_start", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_INTEGER, NULL), &dev::rpc::AdminBrcFace::miner_startI);
                    this->bindAndAddMethod(jsonrpc::Procedure("miner_stop", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN,  NULL), &dev::rpc::AdminBrcFace::miner_stopI);
                    this->bindAndAddMethod(jsonrpc::Procedure("miner_setBrcerbase", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::miner_setBrcerbaseI);
                    this->bindAndAddMethod(jsonrpc::Procedure("miner_setExtra", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::miner_setExtraI);
                    this->bindAndAddMethod(jsonrpc::Procedure("miner_setGasPrice", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING, NULL), &dev::rpc::AdminBrcFace::miner_setGasPriceI);
                    this->bindAndAddMethod(jsonrpc::Procedure("miner_hashrate", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,  NULL), &dev::rpc::AdminBrcFace::miner_hashrateI);
                }

                inline virtual void admin_brc_blockQueueStatusI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_blockQueueStatus(request[0u].asString());
                }
                inline virtual void admin_brc_setAskPriceI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_setAskPrice(request[0u].asString(), request[1u].asString());
                }
                inline virtual void admin_brc_setBidPriceI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_setBidPrice(request[0u].asString(), request[1u].asString());
                }
                inline virtual void admin_brc_setMiningI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_setMining(request[0u].asBool(), request[1u].asString());
                }
                inline virtual void admin_brc_findBlockI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_findBlock(request[0u].asString(), request[1u].asString());
                }
                inline virtual void admin_brc_blockQueueFirstUnknownI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_blockQueueFirstUnknown(request[0u].asString());
                }
                inline virtual void admin_brc_blockQueueRetryUnknownI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_blockQueueRetryUnknown(request[0u].asString());
                }
                inline virtual void admin_brc_allAccountsI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_allAccounts(request[0u].asString());
                }
                inline virtual void admin_brc_newAccountI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_newAccount(request[0u], request[1u].asString());
                }
                inline virtual void admin_brc_setMiningBenefactorI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_setMiningBenefactor(request[0u].asString(), request[1u].asString());
                }
                inline virtual void admin_brc_inspectI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_inspect(request[0u].asString(), request[1u].asString());
                }
                inline virtual void admin_brc_reprocessI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_reprocess(request[0u].asString(), request[1u].asString());
                }
                inline virtual void admin_brc_vmTraceI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_vmTrace(request[0u].asString(), request[1u].asInt(), request[2u].asString());
                }
                inline virtual void admin_brc_getReceiptByHashAndIndexI(const Json::Value &request, Json::Value &response)
                {
                    response = this->admin_brc_getReceiptByHashAndIndex(request[0u].asString(), request[1u].asInt(), request[2u].asString());
                }
                inline virtual void miner_startI(const Json::Value &request, Json::Value &response)
                {
                    response = this->miner_start(request[0u].asInt());
                }
                inline virtual void miner_stopI(const Json::Value &request, Json::Value &response)
                {
                    (void)request;
                    response = this->miner_stop();
                }
                inline virtual void miner_setBrcerbaseI(const Json::Value &request, Json::Value &response)
                {
                    response = this->miner_setBrcerbase(request[0u].asString());
                }
                inline virtual void miner_setExtraI(const Json::Value &request, Json::Value &response)
                {
                    response = this->miner_setExtra(request[0u].asString());
                }
                inline virtual void miner_setGasPriceI(const Json::Value &request, Json::Value &response)
                {
                    response = this->miner_setGasPrice(request[0u].asString());
                }
                inline virtual void miner_hashrateI(const Json::Value &request, Json::Value &response)
                {
                    (void)request;
                    response = this->miner_hashrate();
                }
                virtual Json::Value admin_brc_blockQueueStatus(const std::string& param1) = 0;
                virtual bool admin_brc_setAskPrice(const std::string& param1, const std::string& param2) = 0;
                virtual bool admin_brc_setBidPrice(const std::string& param1, const std::string& param2) = 0;
                virtual bool admin_brc_setMining(bool param1, const std::string& param2) = 0;
                virtual Json::Value admin_brc_findBlock(const std::string& param1, const std::string& param2) = 0;
                virtual std::string admin_brc_blockQueueFirstUnknown(const std::string& param1) = 0;
                virtual bool admin_brc_blockQueueRetryUnknown(const std::string& param1) = 0;
                virtual Json::Value admin_brc_allAccounts(const std::string& param1) = 0;
                virtual Json::Value admin_brc_newAccount(const Json::Value& param1, const std::string& param2) = 0;
                virtual bool admin_brc_setMiningBenefactor(const std::string& param1, const std::string& param2) = 0;
                virtual Json::Value admin_brc_inspect(const std::string& param1, const std::string& param2) = 0;
                virtual Json::Value admin_brc_reprocess(const std::string& param1, const std::string& param2) = 0;
                virtual Json::Value admin_brc_vmTrace(const std::string& param1, int param2, const std::string& param3) = 0;
                virtual Json::Value admin_brc_getReceiptByHashAndIndex(const std::string& param1, int param2, const std::string& param3) = 0;
                virtual bool miner_start(int param1) = 0;
                virtual bool miner_stop() = 0;
                virtual bool miner_setBrcerbase(const std::string& param1) = 0;
                virtual bool miner_setExtra(const std::string& param1) = 0;
                virtual bool miner_setGasPrice(const std::string& param1) = 0;
                virtual std::string miner_hashrate() = 0;
        };

    }
}
#endif //JSONRPC_CPP_STUB_DEV_RPC_ADMINBRCFACE_H_
