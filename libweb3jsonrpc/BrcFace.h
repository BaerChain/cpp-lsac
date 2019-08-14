#ifndef JSONRPC_CPP_STUB_DEV_RPC_BRCFACE_H_
#define JSONRPC_CPP_STUB_DEV_RPC_BRCFACE_H_

#include "ModularServer.h"

namespace dev
{
namespace rpc
{
class BrcFace : public ServerInterface<BrcFace>
{
public:
    BrcFace()
    {
        this->bindAndAddMethod(jsonrpc::Procedure("brc_protocolVersion",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_protocolVersionI);
//        this->bindAndAddMethod(jsonrpc::Procedure("brc_hashrate", jsonrpc::PARAMS_BY_POSITION,
//                                   jsonrpc::JSON_STRING, NULL),
//            &dev::rpc::BrcFace::brc_hashrateI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_coinbase", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_coinbaseI);
//        this->bindAndAddMethod(jsonrpc::Procedure("brc_mining", jsonrpc::PARAMS_BY_POSITION,
//                                   jsonrpc::JSON_BOOLEAN, NULL),
//            &dev::rpc::BrcFace::brc_miningI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_gasPrice", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_gasPriceI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_accounts", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_ARRAY, NULL),
            &dev::rpc::BrcFace::brc_accountsI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_blockNumber", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_blockNumberI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getSuccessPendingOrder",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",
                                   jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getSuccessPendingOrderI);
		this->bindAndAddMethod(jsonrpc::Procedure("brc_getPendingOrderPool",
			jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",
			jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, "param3",
			jsonrpc::JSON_STRING, "param4", jsonrpc::JSON_STRING, NULL),
			&dev::rpc::BrcFace::brc_getPendingOrderPoolI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getPendingOrderPoolForAddr", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, "param2",
                jsonrpc::JSON_STRING, "param3", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getPendingOrderPoolForAddrI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getSuccessPendingOrderForAddr", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, "param2",
                jsonrpc::JSON_STRING, "param3", jsonrpc::JSON_STRING, "param4",
                jsonrpc::JSON_STRING, "param5", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getSuccessPendingOrderForAddrI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getBalance", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,
                "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getBalanceI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getBlockReward", jsonrpc::PARAMS_BY_POSITION, 
                jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, "param2", 
                jsonrpc::JSON_STRING, "param3", jsonrpc::JSON_STRING, "param4", 
                jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getBlockRewardI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getQueryExchangeReward", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, 
                "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getQueryExchangeRewardI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getQueryBlockReward", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, 
                "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getQueryBlockRewardI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getBallot", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,
                "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getBallotI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getStorageAt", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, "param2",
                                   jsonrpc::JSON_STRING, "param3", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getStorageAtI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getStorageRoot", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, "param2",
                                   jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getStorageRootI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getTransactionCount",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",
                                   jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getTransactionCountI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_pendingTransactions",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_ARRAY, NULL),
            &dev::rpc::BrcFace::brc_pendingTransactionsI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getBlockTransactionCountByHash", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getBlockTransactionCountByHashI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getBlockTransactionCountByNumber", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getBlockTransactionCountByNumberI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getUncleCountByBlockHash", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getUncleCountByBlockHashI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getUncleCountByBlockNumber", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getUncleCountByBlockNumberI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getCode", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,
                "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getCodeI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_call", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,
                "param1", jsonrpc::JSON_OBJECT, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_callI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getBlockByHash", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, "param2",
                                   jsonrpc::JSON_BOOLEAN, NULL),
            &dev::rpc::BrcFace::brc_getBlockByHashI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getBlockByNumber",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                   jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_BOOLEAN, NULL),
            &dev::rpc::BrcFace::brc_getBlockByNumberI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getTransactionByHash", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getTransactionByHashI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getAnalysisData", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getAnalysisDataI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getTransactionByBlockHashAndIndex",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                   jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getTransactionByBlockHashAndIndexI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getTransactionByBlockNumberAndIndex",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                   jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getTransactionByBlockNumberAndIndexI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getTransactionReceipt", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getTransactionReceiptI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getUncleByBlockHashAndIndex",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                   jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getUncleByBlockHashAndIndexI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getUncleByBlockNumberAndIndex",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",
                                   jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getUncleByBlockNumberAndIndexI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_newFilter", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_OBJECT, NULL),
            &dev::rpc::BrcFace::brc_newFilterI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_newFilterEx", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_OBJECT, NULL),
            &dev::rpc::BrcFace::brc_newFilterExI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_newBlockFilter", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_newBlockFilterI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_newPendingTransactionFilter",
                                   jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_newPendingTransactionFilterI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_uninstallFilter", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_BOOLEAN, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_uninstallFilterI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getFilterChanges", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getFilterChangesI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getFilterChangesEx", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getFilterChangesExI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getFilterLogs", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getFilterLogsI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_getFilterLogsEx", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_getFilterLogsExI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getLogs", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_OBJECT, NULL),
            &dev::rpc::BrcFace::brc_getLogsI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_getLogsEx", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_OBJECT, NULL),
            &dev::rpc::BrcFace::brc_getLogsExI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_register", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_registerI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_unregister", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_BOOLEAN, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_unregisterI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_fetchQueuedTransactions", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_ARRAY, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_fetchQueuedTransactionsI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_inspectTransaction", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_inspectTransactionI);
        this->bindAndAddMethod(
            jsonrpc::Procedure("brc_sendRawTransaction", jsonrpc::PARAMS_BY_POSITION,
                jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_sendRawTransactionI);

        this->bindAndAddMethod(
                jsonrpc::Procedure("brc_importBlock", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_STRING, NULL),
                &dev::rpc::BrcFace::brc_importBlockI);


        this->bindAndAddMethod(jsonrpc::Procedure("brc_notePassword", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_BOOLEAN, "param1", jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_notePasswordI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_syncing", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_OBJECT, NULL),
            &dev::rpc::BrcFace::brc_syncingI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_estimateGas", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, "param1", jsonrpc::JSON_OBJECT, NULL),
            &dev::rpc::BrcFace::brc_estimateGasI);
        this->bindAndAddMethod(jsonrpc::Procedure("brc_chainId", jsonrpc::PARAMS_BY_POSITION,
                                   jsonrpc::JSON_STRING, NULL),
            &dev::rpc::BrcFace::brc_chainIdI);

		this->bindAndAddMethod(jsonrpc::Procedure("brc_getObtainVote", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL), &dev::rpc::BrcFace::brc_getObtainVoteI);
		this->bindAndAddMethod(jsonrpc::Procedure("brc_getVoted", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, "param2", jsonrpc::JSON_STRING, NULL), &dev::rpc::BrcFace::brc_getVotedI);
		this->bindAndAddMethod(jsonrpc::Procedure("brc_getElector", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1", jsonrpc::JSON_STRING, NULL), &dev::rpc::BrcFace::brc_getElectorI);
           
    }

    inline virtual void brc_protocolVersionI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_protocolVersion();
    }
    inline virtual void brc_coinbaseI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_coinbase();
    }
    inline virtual void brc_gasPriceI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_gasPrice();
    }
    inline virtual void brc_accountsI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_accounts();
    }
    inline virtual void brc_blockNumberI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_blockNumber();
    }
    inline virtual void brc_getPendingOrderPoolForAddrI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getPendingOrderPoolForAddr(
            request[0u].asString(), request[1u].asString(), request[2u].asString());
	}
    inline virtual void brc_getPendingOrderPoolI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getPendingOrderPool(request[0u].asString(), request[1u].asString(),
            request[2u].asString(), request[3u].asString());
    }
	inline virtual void brc_getSuccessPendingOrderI(const Json::Value& request, Json::Value& response)
	{
		response = this->brc_getSuccessPendingOrder(request[0u].asString(), request[1u].asString());
	}
	inline virtual void brc_getSuccessPendingOrderForAddrI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getSuccessPendingOrderForAddr(request[0u].asString(), request[1u].asString(), request[2u].asString(),
                request[3u].asString(), request[4].asString());
    }
    inline virtual void brc_getBalanceI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getBalance(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_getBlockRewardI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getBlockReward(request[0u].asString(), request[1u].asString(), 
            request[2u].asString(), request[3u].asString());
    }
    inline virtual void brc_getQueryExchangeRewardI(const Json::Value& request, Json::Value& response)
    {
            response = this->brc_getQueryExchangeReward(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_getQueryBlockRewardI(const Json::Value& request, Json::Value& response)
    {
            response = this->brc_getQueryBlockReward(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_getBallotI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getBallot(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_getStorageAtI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getStorageAt(
            request[0u].asString(), request[1u].asString(), request[2u].asString());
    }
    inline virtual void brc_getStorageRootI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getStorageRoot(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_getTransactionCountI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getTransactionCount(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_pendingTransactionsI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_pendingTransactions();
    }
    inline virtual void brc_getBlockTransactionCountByHashI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getBlockTransactionCountByHash(request[0u].asString());
    }
    inline virtual void brc_getBlockTransactionCountByNumberI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getBlockTransactionCountByNumber(request[0u].asString());
    }
    inline virtual void brc_getUncleCountByBlockHashI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getUncleCountByBlockHash(request[0u].asString());
    }
    inline virtual void brc_getUncleCountByBlockNumberI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getUncleCountByBlockNumber(request[0u].asString());
    }
    inline virtual void brc_getCodeI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getCode(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_callI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_call(request[0u], request[1u].asString());
    }
    inline virtual void brc_getBlockByHashI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getBlockByHash(request[0u].asString(), request[1u].asBool());
    }
    inline virtual void brc_getBlockByNumberI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getBlockByNumber(request[0u].asString(), request[1u].asBool());
    }
    inline virtual void brc_getTransactionByHashI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getTransactionByHash(request[0u].asString());
    }
    inline virtual void brc_getAnalysisDataI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getAnalysisData(request[0u].asString());
    }
    inline virtual void brc_getTransactionByBlockHashAndIndexI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getTransactionByBlockHashAndIndex(
            request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_getTransactionByBlockNumberAndIndexI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getTransactionByBlockNumberAndIndex(
            request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_getTransactionReceiptI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getTransactionReceipt(request[0u].asString());
    }
    inline virtual void brc_getUncleByBlockHashAndIndexI(
        const Json::Value& request, Json::Value& response)
    {
        response =
            this->brc_getUncleByBlockHashAndIndex(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_getUncleByBlockNumberAndIndexI(
        const Json::Value& request, Json::Value& response)
    {
        response =
            this->brc_getUncleByBlockNumberAndIndex(request[0u].asString(), request[1u].asString());
    }
    inline virtual void brc_newFilterI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_newFilter(request[0u]);
    }
    inline virtual void brc_newFilterExI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_newFilterEx(request[0u]);
    }
    inline virtual void brc_newBlockFilterI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_newBlockFilter();
    }
    inline virtual void brc_newPendingTransactionFilterI(
        const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_newPendingTransactionFilter();
    }
    inline virtual void brc_uninstallFilterI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_uninstallFilter(request[0u].asString());
    }
    inline virtual void brc_getFilterChangesI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getFilterChanges(request[0u].asString());
    }
    inline virtual void brc_getFilterChangesExI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getFilterChangesEx(request[0u].asString());
    }
    inline virtual void brc_getFilterLogsI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getFilterLogs(request[0u].asString());
    }
    inline virtual void brc_getFilterLogsExI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getFilterLogsEx(request[0u].asString());
    }
    inline virtual void brc_getLogsI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getLogs(request[0u]);
    }
    inline virtual void brc_getLogsExI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_getLogsEx(request[0u]);
    }
    inline virtual void brc_registerI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_register(request[0u].asString());
    }
    inline virtual void brc_unregisterI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_unregister(request[0u].asString());
    }
    inline virtual void brc_fetchQueuedTransactionsI(
        const Json::Value& request, Json::Value& response)
    {
        response = this->brc_fetchQueuedTransactions(request[0u].asString());
    }
    inline virtual void brc_inspectTransactionI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_inspectTransaction(request[0u].asString());
    }
    inline virtual void brc_sendRawTransactionI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_sendRawTransaction(request[0u].asString());
    }


    inline virtual void brc_importBlockI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_importBlock(request[0u].asString());
    }

    inline virtual void brc_notePasswordI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_notePassword(request[0u].asString());
    }
    inline virtual void brc_syncingI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_syncing();
    }
    inline virtual void brc_estimateGasI(const Json::Value& request, Json::Value& response)
    {
        response = this->brc_estimateGas(request[0u]);
    }
    inline virtual void brc_chainIdI(const Json::Value& request, Json::Value& response)
    {
        (void)request;
        response = this->brc_chainId();
    }
				inline virtual void brc_getObtainVoteI(const Json::Value &request, Json::Value &response)
				{
					response = this->brc_getObtainVote(request[0u].asString(), request[1u].asString());
				}
				inline virtual void brc_getVotedI(const Json::Value &request, Json::Value &response)
				{
					response = this->brc_getVoted(request[0u].asString(), request[1u].asString());
				}
				inline virtual void brc_getElectorI(const Json::Value &request, Json::Value &response)
				{
					response = this->brc_getElector(request[0u].asString());
				}
    virtual std::string brc_protocolVersion() = 0;
    virtual std::string brc_coinbase() = 0;
    virtual std::string brc_gasPrice() = 0;
    virtual Json::Value brc_accounts() = 0;
    virtual std::string brc_blockNumber() = 0;
    virtual Json::Value brc_getPendingOrderPool(const std::string& param1,
        const std::string& param2, const std::string& param3, const std::string& param4) = 0;
    virtual Json::Value brc_getPendingOrderPoolForAddr(
        const std::string& param1, const std::string& param2, const std::string& param3) = 0;
	virtual Json::Value brc_getSuccessPendingOrder(const std::string& param1, const std::string& param2) = 0;
	virtual Json::Value brc_getSuccessPendingOrderForAddr(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4, const std::string& param5) = 0;
	virtual Json::Value brc_getBalance(const std::string& param1, const std::string& param2) = 0;
    virtual Json::Value brc_getBlockReward(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4) = 0;
    virtual Json::Value brc_getQueryExchangeReward(const std::string& param1, const std::string& param2) = 0;
    virtual Json::Value brc_getQueryBlockReward(const std::string& param1, const std::string& param2) = 0;
    virtual std::string brc_getBallot(const std::string& param1, const std::string& param2) = 0;
    virtual std::string brc_getStorageAt(
        const std::string& param1, const std::string& param2, const std::string& param3) = 0;
    virtual std::string brc_getStorageRoot(
        const std::string& param1, const std::string& param2) = 0;
    virtual std::string brc_getTransactionCount(
        const std::string& param1, const std::string& param2) = 0;
    virtual Json::Value brc_pendingTransactions() = 0;
    virtual Json::Value brc_getBlockTransactionCountByHash(const std::string& param1) = 0;
    virtual Json::Value brc_getBlockTransactionCountByNumber(const std::string& param1) = 0;
    virtual Json::Value brc_getUncleCountByBlockHash(const std::string& param1) = 0;
    virtual Json::Value brc_getUncleCountByBlockNumber(const std::string& param1) = 0;
    virtual std::string brc_getCode(const std::string& param1, const std::string& param2) = 0;
    virtual std::string brc_call(const Json::Value& param1, const std::string& param2) = 0;
    virtual Json::Value brc_getBlockByHash(const std::string& param1, bool param2) = 0;
    virtual Json::Value brc_getBlockByNumber(const std::string& param1, bool param2) = 0;
    virtual Json::Value brc_getTransactionByHash(const std::string& param1) = 0;
    virtual Json::Value brc_getAnalysisData(const std::string& param1) = 0;
    virtual Json::Value brc_getTransactionByBlockHashAndIndex(
        const std::string& param1, const std::string& param2) = 0;
    virtual Json::Value brc_getTransactionByBlockNumberAndIndex(
        const std::string& param1, const std::string& param2) = 0;
    virtual Json::Value brc_getTransactionReceipt(const std::string& param1) = 0;
    virtual Json::Value brc_getUncleByBlockHashAndIndex(
        const std::string& param1, const std::string& param2) = 0;
    virtual Json::Value brc_getUncleByBlockNumberAndIndex(
        const std::string& param1, const std::string& param2) = 0;
    virtual std::string brc_newFilter(const Json::Value& param1) = 0;
    virtual std::string brc_newFilterEx(const Json::Value& param1) = 0;
    virtual std::string brc_newBlockFilter() = 0;
    virtual std::string brc_newPendingTransactionFilter() = 0;
    virtual bool brc_uninstallFilter(const std::string& param1) = 0;
    virtual Json::Value brc_getFilterChanges(const std::string& param1) = 0;
    virtual Json::Value brc_getFilterChangesEx(const std::string& param1) = 0;
    virtual Json::Value brc_getFilterLogs(const std::string& param1) = 0;
    virtual Json::Value brc_getFilterLogsEx(const std::string& param1) = 0;
    virtual Json::Value brc_getLogs(const Json::Value& param1) = 0;
    virtual Json::Value brc_getLogsEx(const Json::Value& param1) = 0;
    virtual std::string brc_register(const std::string& param1) = 0;
    virtual bool brc_unregister(const std::string& param1) = 0;
    virtual Json::Value brc_fetchQueuedTransactions(const std::string& param1) = 0;
    virtual Json::Value brc_inspectTransaction(const std::string& param1) = 0;
    virtual std::string brc_sendRawTransaction(const std::string& param1) = 0;
    virtual std::string brc_importBlock(const std::string& param1) = 0;
    virtual bool brc_notePassword(const std::string& param1) = 0;
    virtual Json::Value brc_syncing() = 0;
    virtual std::string brc_estimateGas(const Json::Value& param1) = 0;
    virtual std::string brc_chainId() = 0;
				virtual Json::Value brc_getObtainVote(const std::string& param1, const std::string& param2) = 0;
				virtual Json::Value brc_getVoted(const std::string& param1, const std::string& param2) = 0;
				virtual Json::Value brc_getElector(const std::string& param1) = 0;
};

}  // namespace rpc
}  // namespace dev
#endif  // JSONRPC_CPP_STUB_DEV_RPC_BRCFACE_H_
