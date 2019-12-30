#pragma once
#include "ModularServer.h"

namespace dev
{
    namespace rpc {
        class BrcV2Face : public ServerInterface<BrcV2Face> {
        public:
            BrcV2Face() {
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getPendingOrderPool",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",
                                                          jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING,
                                                          "param3",
                                                          jsonrpc::JSON_STRING, "param4", jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getPendingOrderPoolI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getPendingOrderPoolForAddr", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, "param2",
                                           jsonrpc::JSON_STRING, "param3", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getPendingOrderPoolForAddrI);

                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getBalance", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,
                                           "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getBalanceI);

                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getQueryExchangeReward", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_STRING,
                                           "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getQueryExchangeRewardI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getQueryBlockReward", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,
                                           "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getQueryBlockRewardI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getBallot", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,
                                           "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getBallotI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getStorageAt", jsonrpc::PARAMS_BY_POSITION,
                                                          jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING,
                                                          "param2",
                                                          jsonrpc::JSON_STRING, "param3", jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getStorageAtI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getStorageRoot", jsonrpc::PARAMS_BY_POSITION,
                                                          jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING,
                                                          "param2",
                                                          jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getStorageRootI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getTransactionCount",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",
                                                          jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getTransactionCountI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_pendingTransactions",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_ARRAY, NULL),
                                       &dev::rpc::BrcV2Face::brc_pendingTransactionsI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getBlockByHash", jsonrpc::PARAMS_BY_POSITION,
                                                          jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING,
                                                          "param2",
                                                          jsonrpc::JSON_BOOLEAN, NULL),
                                       &dev::rpc::BrcV2Face::brc_getBlockByHashI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getBlockDetialByHash", jsonrpc::PARAMS_BY_POSITION,
                                                          jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING,
                                                          "param2",
                                                          jsonrpc::JSON_BOOLEAN, NULL),
                                       &dev::rpc::BrcV2Face::brc_getBlockDetialByHashI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getBlockByNumber",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                                          jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_BOOLEAN, NULL),
                                       &dev::rpc::BrcV2Face::brc_getBlockByNumberI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getBlockDetialByNumber",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                                          jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_BOOLEAN, NULL),
                                       &dev::rpc::BrcV2Face::brc_getBlockDetialByNumberI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getTransactionByHash", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getTransactionByHashI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getTransactionDetialByHash", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getTransactionDetialByHashI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getAnalysisData", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getAnalysisDataI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getTransactionByBlockHashAndIndex",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                                          jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getTransactionByBlockHashAndIndexI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getTransactionDetialByBlockHashAndIndex",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                                          jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getTransactionDetialByBlockHashAndIndexI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getTransactionByBlockNumberAndIndex",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                                          jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getTransactionByBlockNumberAndIndexI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getTransactionDetialByBlockNumberAndIndex",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                                          jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getTransactionDetialByBlockNumberAndIndexI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getTransactionReceipt", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getTransactionReceiptI);

                this->bindAndAddMethod(jsonrpc::Procedure("brc_newPendingTransactionFilter",
                                                          jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_newPendingTransactionFilterI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getFilterChangesEx", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getFilterChangesExI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getFilterLogs", jsonrpc::PARAMS_BY_POSITION,
                                                          jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
                                       &dev::rpc::BrcV2Face::brc_getFilterLogsI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getFilterLogsEx", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getFilterLogsExI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getLogs", jsonrpc::PARAMS_BY_POSITION,
                                                          jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_OBJECT, NULL),
                                       &dev::rpc::BrcV2Face::brc_getLogsI);
                this->bindAndAddMethod(jsonrpc::Procedure("brc_getLogsEx", jsonrpc::PARAMS_BY_POSITION,
                                                          jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_OBJECT, NULL),
                                       &dev::rpc::BrcV2Face::brc_getLogsExI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_fetchQueuedTransactions", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_fetchQueuedTransactionsI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_inspectTransaction", jsonrpc::PARAMS_BY_POSITION,
                                           jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_inspectTransactionI);

                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getObtainVote", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT,
                                           "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getObtainVoteI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getVoted", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                           jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getVotedI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_getElector", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT,
                                           "param1", jsonrpc::JSON_STRING, NULL),
                        &dev::rpc::BrcV2Face::brc_getElectorI);
                this->bindAndAddMethod(
                        jsonrpc::Procedure("brc_estimateGasUsed", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,
                                           "param1", jsonrpc::JSON_OBJECT, NULL),
                        &dev::rpc::BrcV2Face::brc_estimateGasUsedI);
            }

            inline virtual void brc_getPendingOrderPoolForAddrI(
                    const Json::Value &request, Json::Value &response) {
                response = this->brc_getPendingOrderPoolForAddr(
                        request[0u].asString(), request[1u].asString(), request[2u].asString());
            }

            inline virtual void brc_getPendingOrderPoolI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getPendingOrderPool(request[0u].asString(), request[1u].asString(),
                                                         request[2u].asString(), request[3u].asString());
            }

            inline virtual void brc_getBalanceI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getBalance(request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getQueryExchangeRewardI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getQueryExchangeReward(request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getQueryBlockRewardI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getQueryBlockReward(request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getBallotI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getBallot(request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getStorageAtI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getStorageAt(
                        request[0u].asString(), request[1u].asString(), request[2u].asString());
            }

            inline virtual void brc_getStorageRootI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getStorageRoot(request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getTransactionCountI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getTransactionCount(request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_pendingTransactionsI(const Json::Value &request, Json::Value &response) {
                (void) request;
                response = this->brc_pendingTransactions();
            }

            inline virtual void brc_getBlockByHashI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getBlockByHash(request[0u].asString(), request[1u].asBool());
            }

            inline virtual void brc_getBlockDetialByHashI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getBlockDetialByHash(request[0u].asString(), request[1].asBool());
            }

            inline virtual void brc_getBlockByNumberI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getBlockByNumber(request[0u].asString(), request[1u].asBool());
            }

            inline virtual void brc_getBlockDetialByNumberI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getBlockDetialByNumber(request[0u].asString(), request[1u].asBool());
            }

            inline virtual void brc_getTransactionByHashI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getTransactionByHash(request[0u].asString());
            }

            inline virtual void brc_getTransactionDetialByHashI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getTransactionDetialByHash(request[0u].asString());
            }

            inline virtual void brc_getAnalysisDataI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getAnalysisData(request[0u].asString());
            }

            inline virtual void brc_getTransactionByBlockHashAndIndexI(
                    const Json::Value &request, Json::Value &response) {
                response = this->brc_getTransactionByBlockHashAndIndex(
                        request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getTransactionDetialByBlockHashAndIndexI(
                    const Json::Value &request, Json::Value &response) {
                response = this->brc_getTransactionDetialByBlockHashAndIndex(
                        request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getTransactionByBlockNumberAndIndexI(
                    const Json::Value &request, Json::Value &response) {
                response = this->brc_getTransactionByBlockNumberAndIndex(
                        request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getTransactionDetialByBlockNumberAndIndexI(
                    const Json::Value &request, Json::Value &response) {
                response = this->brc_getTransactionDetialByBlockNumberAndIndex(
                        request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getTransactionReceiptI(
                    const Json::Value &request, Json::Value &response) {
                response = this->brc_getTransactionReceipt(request[0u].asString());
            }

            inline virtual void brc_newPendingTransactionFilterI(
                    const Json::Value &request, Json::Value &response) {
                (void) request;
                response = this->brc_newPendingTransactionFilter();
            }

            inline virtual void brc_getFilterChangesExI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getFilterChangesEx(request[0u].asString());
            }

            inline virtual void brc_getFilterLogsI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getFilterLogs(request[0u].asString());
            }

            inline virtual void brc_getFilterLogsExI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getFilterLogsEx(request[0u].asString());
            }

            inline virtual void brc_getLogsI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getLogs(request[0u]);
            }

            inline virtual void brc_getLogsExI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getLogsEx(request[0u]);
            }

            inline virtual void brc_fetchQueuedTransactionsI(
                    const Json::Value &request, Json::Value &response) {
                response = this->brc_fetchQueuedTransactions(request[0u].asString());
            }

            inline virtual void brc_inspectTransactionI(const Json::Value &request, Json::Value &response) {
                response = this->brc_inspectTransaction(request[0u].asString());
            }

            inline virtual void brc_estimateGasUsedI(const Json::Value &request, Json::Value &response) {
                response = this->brc_estimateGasUsed(request[0u]);
            }

            inline virtual void brc_getObtainVoteI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getObtainVote(request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getVotedI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getVoted(request[0u].asString(), request[1u].asString());
            }

            inline virtual void brc_getElectorI(const Json::Value &request, Json::Value &response) {
                response = this->brc_getElector(request[0u].asString());
            }

            virtual Json::Value brc_getPendingOrderPool(const std::string &param1,
                                                        const std::string &param2, const std::string &param3,
                                                        const std::string &param4) = 0;

            virtual Json::Value brc_getPendingOrderPoolForAddr(
                    const std::string &param1, const std::string &param2, const std::string &param3) = 0;

            virtual Json::Value brc_getBalance(const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_getQueryExchangeReward(const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_getQueryBlockReward(const std::string &param1, const std::string &param2) = 0;

            virtual std::string brc_getBallot(const std::string &param1, const std::string &param2) = 0;

            virtual std::string brc_getStorageAt(
                    const std::string &param1, const std::string &param2, const std::string &param3) = 0;

            virtual std::string brc_getStorageRoot(
                    const std::string &param1, const std::string &param2) = 0;

            virtual std::string brc_getTransactionCount(
                    const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_pendingTransactions() = 0;

            virtual Json::Value brc_getBlockByHash(const std::string &param1, bool param2) = 0;

            virtual Json::Value brc_getBlockDetialByHash(const std::string &param1, bool param2) = 0;

            virtual Json::Value brc_getBlockByNumber(const std::string &param1, bool param2) = 0;

            virtual Json::Value brc_getBlockDetialByNumber(const std::string &param1, bool param2) = 0;

            virtual Json::Value brc_getTransactionByHash(const std::string &param1) = 0;

            virtual Json::Value brc_getTransactionDetialByHash(const std::string &param1) = 0;

            virtual Json::Value brc_getAnalysisData(const std::string &param1) = 0;

            virtual Json::Value brc_getTransactionByBlockHashAndIndex(
                    const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_getTransactionDetialByBlockHashAndIndex(
                    const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_getTransactionByBlockNumberAndIndex(
                    const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_getTransactionDetialByBlockNumberAndIndex(
                    const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_getTransactionReceipt(const std::string &param1) = 0;

            virtual std::string brc_newPendingTransactionFilter() = 0;

            virtual Json::Value brc_getFilterChangesEx(const std::string &param1) = 0;

            virtual Json::Value brc_getFilterLogs(const std::string &param1) = 0;

            virtual Json::Value brc_getFilterLogsEx(const std::string &param1) = 0;

            virtual Json::Value brc_getLogs(const Json::Value &param1) = 0;

            virtual Json::Value brc_getLogsEx(const Json::Value &param1) = 0;

            virtual Json::Value brc_fetchQueuedTransactions(const std::string &param1) = 0;

            virtual Json::Value brc_inspectTransaction(const std::string &param1) = 0;

            virtual Json::Value brc_estimateGasUsed(const Json::Value &param1) = 0;

            virtual Json::Value brc_getObtainVote(const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_getVoted(const std::string &param1, const std::string &param2) = 0;

            virtual Json::Value brc_getElector(const std::string &param1) = 0;
        };

    }  // namespace rpc
}  // namespace dev
