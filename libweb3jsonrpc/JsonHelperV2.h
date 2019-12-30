#pragma once

#include <json/json.h>
#include <libp2p/Common.h>
#include <libbrccore/Common.h>
#include <libbrccore/BlockHeader.h>
#include <libbrcdchain/LogFilter.h>
#include <libbrcdchain/Transaction.h>
#include <indexDb/database/include/brc/objects.hpp>
namespace dev
{

    Json::Value toJsonV2(std::map<h256, std::pair<u256, u256>> const& _storage);
    Json::Value toJsonV2(std::unordered_map<u256, u256> const& _storage);
    Json::Value toJsonV2(Address const& _address);

    namespace p2p
    {

        Json::Value toJsonV2(PeerSessionInfo const& _p);

    }

    namespace brc
    {

        class Transaction;
        class LocalisedTransaction;
        class SealEngineFace;
        struct BlockDetails;
        class Interface;
        using Transactions = std::vector<Transaction>;
        using UncleHashes = h256s;
        using TransactionHashes = h256s;

        struct accountStu;
        struct voteStu;
        struct electorStu;
        //struct exchange_order;

        Json::Value toJsonV2(BlockHeader const& _bi, SealEngineFace* _face = nullptr);
//TODO: wrap these params into one structure eg. "LocalisedTransaction"
        Json::Value toJsonV2(Transaction const& _t, std::pair<h256, unsigned> _location, BlockNumber _blockNumber, bool _detialStatus, SealEngineFace* _face = nullptr);
        Json::Value toJsonV2(BlockHeader const& _bi, BlockDetails const& _bd, UncleHashes const& _us, Transactions const& _ts,bool _detialStatus, SealEngineFace* _face = nullptr);
        Json::Value toJsonV2(BlockHeader const& _bi, BlockDetails const& _bd, UncleHashes const& _us, TransactionHashes const& _ts, SealEngineFace* _face = nullptr);
        Json::Value toJsonV2(TransactionSkeleton const& _t);
        Json::Value toJsonV2(Transaction const& _t);
        Json::Value toJsonV2(Transaction const& _t, bytes const& _rlp);
        Json::Value toJsonV2(LocalisedTransaction const& _t, bool _detialStatus, SealEngineFace* _face = nullptr);
        Json::Value toJsonV2(TransactionReceipt const& _t);
        Json::Value toJsonV2(LocalisedTransactionReceipt const& _t);
        Json::Value toJsonV2(LocalisedLogEntry const& _e);
        Json::Value toJsonV2(LogEntry const& _e);
        Json::Value toJsonV2(std::unordered_map<h256, LocalisedLogEntries> const& _entriesByBlock);

        Json::Value toJsonV2(accountStu const& _e);
        Json::Value toJsonV2(voteStu const& _e);
        Json::Value toJsonV2(electorStu const& _e);
        Json::Value toJsonV2(dev::brc::ex::exchange_order const& _e);

        Json::Value toJsonV2ByBlock(LocalisedLogEntries const& _entries);
        TransactionSkeleton toTransactionSkeletonV2(Json::Value const& _json);
        LogFilter toLogFilterV2(Json::Value const& _json);
        LogFilter toLogFilterV2(Json::Value const& _json, Interface const& _client);	// commented to avoid warning. Uncomment once in use @ PoC-7.
        Json::Value analysisDataV2(bytes const& _data);

	    std::tuple<std::string, std::string, std::string> enumToString(ex::order_type _type, ex::order_token_type _token_type, ex::order_buy_type _buy_type);

    }

//    namespace rpc
//    {
//        h256 h256fromHex(std::string const& _s);
//    }

    template <class T>
    Json::Value toJsonV2(std::vector<T> const& _es)
    {
        Json::Value res(Json::arrayValue);
        for (auto const& e: _es)
            res.append(toJsonV2(e));
        return res;
    }

    template < >
    Json::Value toJsonV2(std::vector<dev::brc::ex::exchange_order> const& _es)
    {
        Json::Value res(Json::arrayValue);
        for (auto const& e: _es)
            res.append(brc::toJsonV2(e));
        return res;
    }

    template <class T>
    Json::Value toJsonV2(std::unordered_set<T> const& _es)
    {
        Json::Value res(Json::arrayValue);
        for (auto const& e: _es)
            res.append(toJsonV2(e));
        return res;
    }

    template <class T>
    Json::Value toJsonV2(std::set<T> const& _es)
    {
        Json::Value res(Json::arrayValue);
        for (auto const& e: _es)
            res.append(toJsonV2(e));
        return res;
    }

}
