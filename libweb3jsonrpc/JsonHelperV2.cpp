#include "JsonHelperV2.h"

#include <jsonrpccpp/common/exception.h>
#include <libbrccore/CommonJS.h>
#include <libbrccore/SealEngine.h>
#include <libbrcdchain/Client.h>
#include <libwebthree/WebThree.h>
#include <libbrcdchain/Transaction.h>
#include <indexDb/database/include/brc/objects.hpp>

using namespace std;
using namespace dev;
using namespace brc;
using namespace dev::brc::ex;

namespace dev {
    Json::Value toJsonV2(unordered_map<u256, u256> const &_storage) {
        Json::Value res(Json::objectValue);
        for (auto i : _storage)
            res[toJS(i.first)] = toJS(i.second);
        return res;
    }

    Json::Value toJsonV2(map<h256, pair<u256, u256>> const &_storage) {
        Json::Value res(Json::objectValue);
        for (auto i : _storage)
            res[toJS(u256(i.second.first))] = toJS(i.second.second);
        return res;
    }

    Json::Value toJsonV2(Address const &_address) {
        return toJS(_address);
    }

// ////////////////////////////////////////////////////////////////////////////////
// p2p
// ////////////////////////////////////////////////////////////////////////////////
    namespace p2p {
        Json::Value toJsonV2(p2p::PeerSessionInfo const &_p) {
            //@todo localAddress
            //@todo protocols
            Json::Value ret;
            ret["id"] = _p.id.hex();
            ret["name"] = _p.clientVersion;
            ret["network"]["remoteAddress"] = _p.host + ":" + toString(_p.port);
            ret["lastPing"] = (int) chrono::duration_cast<chrono::milliseconds>(_p.lastPing).count();
            for (auto const &i : _p.notes)
                ret["notes"][i.first] = i.second;
            for (auto const &i : _p.caps)
                ret["caps"].append(i.first + "/" + toString((unsigned) i.second));
            return ret;
        }

    }  // namespace p2p

// ////////////////////////////////////////////////////////////////////////////////
// brc
// ////////////////////////////////////////////////////////////////////////////////

    namespace brc {

        std::tuple<std::string, std::string, std::string> enumToString(ex::order_type type, ex::order_token_type token_type, ex::order_buy_type buy_type) {
            std::string _type, _token_type, _buy_type;
            switch (type) {
            case dev::brc::ex::order_type::sell:
                _type = std::string("sell");
                break;
            case dev::brc::ex::order_type::buy:
                _type = std::string("buy");
                break;
            default:
                _type = std::string("NULL");
                break;
            }

            switch (token_type) {
            case dev::brc::ex::order_token_type::BRC:
                _token_type = std::string("BRC");
                break;
            case dev::brc::ex::order_token_type::FUEL:
                _token_type = std::string("FUEL");
                break;
            default:
                _token_type = std::string("NULL");
                break;
            }

            switch (buy_type) {
                case dev::brc::ex::order_buy_type::all_price:
                    _buy_type = std::string("all_price");
                    break;
                case dev::brc::ex::order_buy_type::only_price:
                    _buy_type = std::string("only_price");
                    break;
                default:
                    _buy_type = std::string("NULL");
                break;
            }

        std::tuple<std::string, std::string, std::string> _result = std::make_tuple(_type, _token_type, _buy_type);
        return _result;
    }  

        Json::Value toJsonV2(dev::brc::BlockHeader const &_bi, SealEngineFace *_sealer) {
            Json::Value res;
            if (_bi) {
                DEV_IGNORE_EXCEPTIONS(res["hash"] = toJS(_bi.hash()));
                res["parentHash"] = toJS(_bi.parentHash());
                res["sha3Uncles"] = toJS(_bi.sha3Uncles());
                res["author"] = jsToNewAddress(_bi.author());
                res["stateRoot"] = toJS(_bi.stateRoot());
                res["transactionsRoot"] = toJS(_bi.transactionsRoot());
                res["receiptsRoot"] = toJS(_bi.receiptsRoot());
                res["number"] = toJS(_bi.number());
                res["gasUsed"] = toJS(_bi.gasUsed());
                res["gasLimit"] = toJS(_bi.gasLimit());
                res["extraData"] = toJS(_bi.extraData());
                res["logsBloom"] = toJS(_bi.logBloom());
                res["timestamp"] = toJS(_bi.timestamp());

                res["sign"] = toJS((h520) _bi.sign_data());


                // TODO: remove once JSONRPC spec is updated to use "author" over "miner".
                res["miner"] = toJS(_bi.author());
                if (_sealer)
                    for (auto const &i : _sealer->jsInfo(_bi))
                        res[i.first] = i.second;
            }
            return res;
        }




        Json::Value toJsonV2(
                dev::brc::Transaction const &_t, std::pair<h256, unsigned> _location, BlockNumber _blockNumber,bool _detialStatus, SealEngineFace* _face) {
            Json::Value res;
            if (_t) {
                res["hash"] = toJS(_t.sha3());
                res["input"] = toJS(_t.data());
                res["to"] = _t.isCreation() ? Json::Value() : jsToNewAddress(_t.receiveAddress());
                res["from"] = jsToNewAddress(_t.safeSender());
                res["gas"] = toJS(_t.gas());
                if( _face != nullptr)
                {
                    res["baseGas"] = toJS(_t.baseGasRequired(_face->brcSchedule(u256(_blockNumber))));
                }
                res["gasPrice"] = toJS(_t.gasPrice());
                res["nonce"] = toJS(_t.nonce());
                res["value"] = toJS(_t.value());
                res["blockHash"] = toJS(_location.first);
                res["transactionIndex"] = toJS(_location.second);
                res["blockNumber"] = toJS(_blockNumber);
                res["v"] = toJS(_t.signature().v);
                res["r"] = toJS(_t.signature().r);
                res["s"] = toJS(_t.signature().s);
                if(_detialStatus == true && _t.isCreation() != true && _t.type() != TransactionBase::MessageCall)
                {
                    res["txdata"] = analysisDataV2(_t.data());

                }
                res["transactionRlp"] = toJS(_t.rlp());
            }
            return res;
        }

        Json::Value toJsonV2(dev::brc::BlockHeader const &_bi, BlockDetails const &_bd,
                           UncleHashes const &_us, Transactions const &_ts, bool _detialStatus, SealEngineFace *_face) {
            Json::Value res = toJsonV2(_bi, _face);
            if (_bi) {
                res["totalDifficulty"] = toJS(_bd.totalDifficulty);
                res["size"] = toJS(_bd.size);
                res["uncles"] = Json::Value(Json::arrayValue);
                for (h256 h : _us)
                    res["uncles"].append(toJS(h));
                res["transactions"] = Json::Value(Json::arrayValue);
                for (unsigned i = 0; i < _ts.size(); i++)
                    res["transactions"].append(
                            toJsonV2(_ts[i], std::make_pair(_bi.hash(), i), (BlockNumber) _bi.number(), _detialStatus, _face));
            }
            return res;
        }

        Json::Value toJsonV2(dev::brc::BlockHeader const &_bi, BlockDetails const &_bd,
                           UncleHashes const &_us, TransactionHashes const &_ts, SealEngineFace *_face) {
            Json::Value res = toJsonV2(_bi, _face);
            if (_bi) {
                res["totalDifficulty"] = toJS(_bd.totalDifficulty);
                res["size"] = toJS(_bd.size);
                res["uncles"] = Json::Value(Json::arrayValue);
                for (h256 h : _us)
                    res["uncles"].append(toJS(h));
                res["transactions"] = Json::Value(Json::arrayValue);
                for (h256 const &t : _ts)
                    res["transactions"].append(toJS(t));
            }
            return res;
        }

        Json::Value toJsonV2(dev::brc::TransactionSkeleton const &_t) {
            Json::Value res;
            res["to"] = _t.creation ? Json::Value() : jsToNewAddress(_t.to);
            res["from"] = jsToNewAddress(_t.from);
            res["gas"] = toJS(_t.gas);
            res["gasPrice"] = toJS(_t.gasPrice);
            res["value"] = toJS(_t.value);
            res["data"] = toJS(_t.data, 32);
            return res;
        }

        Json::Value toJsonV2(dev::brc::TransactionReceipt const &_t) {
            Json::Value res;
            if (_t.hasStatusCode())
                res["status"] = toString(_t.statusCode());
            else
                res["stateRoot"] = toJS(_t.stateRoot());
            res["gasUsed"] = toJS(_t.cumulativeGasUsed());
            res["bloom"] = toJS(_t.bloom());
            res["log"] = dev::toJsonV2(_t.log());
            return res;
        }

        Json::Value toJsonV2(dev::brc::LocalisedTransactionReceipt const &_t) {
            Json::Value res;
            res["transactionHash"] = toJS(_t.hash());
            res["transactionIndex"] = _t.transactionIndex();
            res["blockHash"] = toJS(_t.blockHash());
            res["blockNumber"] = _t.blockNumber();
            res["from"] = jsToNewAddress(_t.from());
            res["to"] = jsToNewAddress(_t.to());
            res["cumulativeGasUsed"] = toJS(_t.cumulativeGasUsed());
//            if(_face != nullptr)
//            {
//                res["baseGas"] = toJS(_t.baseGasRequired(_face->brcSchedule(u256(_t.blockNumber()))));
//            }
            res["gasUsed"] = toJS(_t.gasUsed());
            res["contractAddress"] = toJS(_t.contractAddress());
            res["logs"] = dev::toJsonV2(_t.localisedLogs());
            res["logsBloom"] = toJS(_t.bloom());
            if (_t.hasStatusCode())
                res["status"] = toString(_t.statusCode());
            else
                res["stateRoot"] = toJS(_t.stateRoot());
            return res;
        }

        Json::Value toJsonV2(dev::brc::Transaction const &_t) {
            Json::Value res;
            res["to"] = _t.isCreation() ? Json::Value() : jsToNewAddress(_t.to());
            res["from"] = jsToNewAddress(_t.from());
            res["gas"] = toJS(_t.gas());
            res["gasPrice"] = toJS(_t.gasPrice());
            res["value"] = toJS(_t.value());
            res["data"] = toJS(_t.data(), _t.data().size());
            res["nonce"] = toJS(_t.nonce());
            res["hash"] = toJS(_t.sha3(WithSignature));
            res["sighash"] = toJS(_t.sha3(WithoutSignature));
            res["r"] = toJS(_t.signature().r);
            res["s"] = toJS(_t.signature().s);
            res["v"] = toJS(_t.signature().v);
            return res;
        }

        Json::Value toJsonV2(dev::brc::Transaction const &_t, bytes const &_rlp) {
            Json::Value res;
            res["raw"] = toJS(_rlp);
            res["tx"] = toJsonV2(_t);
            return res;
        }

        Json::Value toJsonV2(dev::brc::LocalisedTransaction const &_t, bool _detialStatus, SealEngineFace* _face) {
            Json::Value res;
            if (_t) {
                res["hash"] = toJS(_t.sha3());
                res["input"] = toJS(_t.data());
                res["to"] = _t.isCreation() ? Json::Value() : jsToNewAddress(_t.receiveAddress());
                res["from"] = jsToNewAddress(_t.safeSender());
                if(_face != nullptr)
                {
                    res["baseGas"] = toJS(_t.baseGasRequired(_face->brcSchedule(u256(_t.blockNumber()))));
                }
                res["gas"] = toJS(_t.gas());
                res["gasPrice"] = toJS(_t.gasPrice());
                res["nonce"] = toJS(_t.nonce());
                res["value"] = toJS(_t.value());
                res["blockHash"] = toJS(_t.blockHash());
                res["transactionIndex"] = toJS(_t.transactionIndex());
                res["blockNumber"] = toJS(_t.blockNumber());
                if(_detialStatus == true && _t.isCreation() != true && _t.type() != TransactionBase::MessageCall)
                {
                    res["txdata"] = analysisDataV2(_t.data());
                }
            }
            return res;
        }

        Json::Value analysisDataV2(bytes const& _data) {
            try{
                RLP _r(_data);
                Json::Value _JsArray;
                std::vector<bytes> _ops = _r.toVector<bytes>();
                for (auto val : _ops) {
                    dev::brc::transationTool::op_type _type =  dev::brc::transationTool::operation::get_type(val);
                    if(_type == dev::brc::transationTool::brcTranscation) {
                        Json::Value res;
                        dev::brc::transationTool::transcation_operation _transation_op = dev::brc::transationTool::transcation_operation(val);
                        res["type"] = toJS(_type);
                        if(_transation_op.m_from != Address(0))
                            res["from"] = jsToNewAddress(_transation_op.m_from);
                        res["to"] = jsToNewAddress(_transation_op.m_to);
                        res["transcation_type"] = toJS(_transation_op.m_Transcation_type);
                        res["transcation_numbers"] = toJS(_transation_op.m_Transcation_numbers);
                        _JsArray.append(res);
                    }
                    if(_type == dev::brc::transationTool::vote) {
                        Json::Value res;
                        dev::brc::transationTool::vote_operation _vote_op = dev::brc::transationTool::vote_operation(val);
                        res["type"] = toJS(_type);
                        if(_vote_op.m_from != Address(0))
                            res["from"] = jsToNewAddress(_vote_op.m_from);
                        res["to"] = jsToNewAddress(_vote_op.m_to);
                        res["vote_type"] = toJS(_vote_op.m_vote_type);
                        res["vote_numbers"] = toJS(_vote_op.m_vote_numbers);
                        _JsArray.append(res);
                    }
                    if(_type == dev::brc::transationTool::pendingOrder) {
                        Json::Value res;
                        dev::brc::transationTool::pendingorder_opearaion _pendering_op = dev::brc::transationTool::pendingorder_opearaion(val);
                        uint8_t m_Pendingorder_type = (uint8_t)_pendering_op.m_Pendingorder_type;
                        uint8_t m_Pendingorder_Token_type = (uint8_t)_pendering_op.m_Pendingorder_Token_type;
                        uint8_t m_Pendingorder_buy_type = (uint8_t)_pendering_op.m_Pendingorder_buy_type;

                        res["type"] = toJS(_type);
                        res["from"] = jsToNewAddress(_pendering_op.m_from);
                        res["pendingorder_type"] = toJS(m_Pendingorder_type);
                        res["token_type"] = toJS(m_Pendingorder_Token_type);
                        res["buy_type"] = toJS(m_Pendingorder_buy_type);
                        res["num"] = toJS(_pendering_op.m_Pendingorder_num);
                        res["price"] = toJS(_pendering_op.m_Pendingorder_price);
                        _JsArray.append(res);
                    }
                    if(_type == dev::brc::transationTool::cancelPendingOrder) {
                        Json::Value res;
                        dev::brc::transationTool::cancelPendingorder_operation _cancel_op = dev::brc::transationTool::cancelPendingorder_operation(val);
                        res["type"] = toJS(_type);
                        res["cancelType"] = toJS(_cancel_op.m_cancelType);
                        res["hash"] = toJS(_cancel_op.m_hash);
                        _JsArray.append(res);
                    }
                    if(_type == dev::brc::transationTool::receivingincome) {
                        Json::Value res;
                        dev::brc::transationTool::receivingincome_operation _op = dev::brc::transationTool::receivingincome_operation(val);
                        res["type"] = toJS(_type);
                        res["receivingType"] = toJS(_op.m_receivingType);
                        res["from"] = jsToNewAddress(_op.m_from);
                        _JsArray.append(res);
                    }
                    if(_type == dev::brc::transationTool::transferAutoEx) {
                        Json::Value res;
                        dev::brc::transationTool::transferAutoEx_operation _op = dev::brc::transationTool::transferAutoEx_operation(val);
                        res["type"] = toJS(_type);
                        res["autoExType"] = toJS(_op.m_autoExType);
                        res["autoExNum"] = toJS(_op.m_autoExNum);
                        res["transferNum"] = toJS(_op.m_transferNum);
                        res["from"] = jsToNewAddress(_op.m_from);
                        res["to"] = jsToNewAddress(_op.m_to);
                        _JsArray.append(res);
                    }
                }
                return _JsArray;
            }
            catch (...) {
                throw jsonrpc::JsonRpcException("Invalid data format!");
            }

        }

        Json::Value toJsonV2(dev::brc::LocalisedLogEntry const &_e) {
            Json::Value res;

            if (_e.isSpecial)
                res = toJS(_e.special);
            else {
                res = toJsonV2(static_cast<dev::brc::LogEntry const &>(_e));
                res["polarity"] = _e.polarity == BlockPolarity::Live ? true : false;
                if (_e.mined) {
                    res["type"] = "mined";
                    res["blockNumber"] = _e.blockNumber;
                    res["blockHash"] = toJS(_e.blockHash);
                    res["logIndex"] = _e.logIndex;
                    res["transactionHash"] = toJS(_e.transactionHash);
                    res["transactionIndex"] = _e.transactionIndex;
                } else {
                    res["type"] = "pending";
                    res["blockNumber"] = Json::Value(Json::nullValue);
                    res["blockHash"] = Json::Value(Json::nullValue);
                    res["logIndex"] = Json::Value(Json::nullValue);
                    res["transactionHash"] = Json::Value(Json::nullValue);
                    res["transactionIndex"] = Json::Value(Json::nullValue);
                }
            }
            return res;
        }

        Json::Value toJsonV2(dev::brc::LogEntry const &_e) {
            Json::Value res;
            res["data"] = toJS(_e.data);
            res["address"] = jsToNewAddress(_e.address);
            res["topics"] = Json::Value(Json::arrayValue);
            for (auto const &t : _e.topics)
                res["topics"].append(toJS(t));
            return res;
        }

        Json::Value toJsonV2(std::unordered_map<h256, dev::brc::LocalisedLogEntries> const &_entriesByBlock,
                           vector<h256> const &_order) {
            Json::Value res(Json::arrayValue);
            for (auto const &i : _order) {
                auto entries = _entriesByBlock.at(i);
                Json::Value currentBlock(Json::objectValue);
                LocalisedLogEntry entry = entries[0];
                if (entry.mined) {
                    currentBlock["blockNumber"] = entry.blockNumber;
                    currentBlock["blockHash"] = toJS(entry.blockHash);
                    currentBlock["type"] = "mined";
                } else
                    currentBlock["type"] = "pending";

                currentBlock["polarity"] = entry.polarity == BlockPolarity::Live ? true : false;
                currentBlock["logs"] = Json::Value(Json::arrayValue);

                for (LocalisedLogEntry const &e : entries) {
                    Json::Value log(Json::objectValue);
                    log["logIndex"] = e.logIndex;
                    log["transactionIndex"] = e.transactionIndex;
                    log["transactionHash"] = toJS(e.transactionHash);
                    log["address"] = jsToNewAddress(e.address);
                    log["data"] = toJS(e.data);
                    log["topics"] = Json::Value(Json::arrayValue);
                    for (auto const &t : e.topics)
                        log["topics"].append(toJS(t));

                    currentBlock["logs"].append(log);
                }

                res.append(currentBlock);
            }

            return res;
        }

        Json::Value toJsonV2(accountStu const& _e) {
            Json::Value jv;
            if(_e.isNull())
                return jv;
            jv["Address"] = jsToNewAddress(_e.m_addr);
            jv["balance"] = toJS(_e.m_cookie);
            jv["FBalance"] = toJS(_e.m_fcookie);
            jv["BRC"] = toJS(_e.m_brc);
            jv["FBRC"] = toJS(_e.m_fbrc);
            jv["vote"] = toJS(_e.m_voteAll);
            jv["ballot"] = toJS(_e.m_ballot);
            jv["poll"] = toJS(_e.m_poll);
            jv["nonce"] = toJS(_e.m_nonce);
            jv["cookieinsummury"] = toJS(_e.m_cookieInSummury);
            Json::Value _array;
            for(auto const& a : _e.m_vote){
                Json::Value _v;
                _v["Address"] = jsToNewAddress(a.first);
                _v["vote_num"] = toJS(a.second);
                _array.append(_v);
            }
            jv["vote"] = _array;

            return jv;
        }

        Json::Value toJsonV2(voteStu const& _e) {
            Json::Value jv;
            if(_e.isNull())
                return jv;
            Json::Value _arry;
            for(auto const& a: _e.m_vote){
                Json::Value _v;
                _v["address"] = jsToNewAddress(a.first);
                _v["voted_num"] = toJS(a.second);
                _arry.append(_v);
            }
            jv["vote"] = _arry;
            jv["total_voted_num"] = toJS(_e.m_totalVoteNum);
            return jv;
        }

        Json::Value toJsonV2(electorStu const& _e) {
            Json::Value jv;
            Json::Value _arry;
            if(_e.isNull())
                return jv;
            if(_e.m_isZeroAddr){
                for(auto const& a: _e.m_elector){
                    Json::Value _v;
                    _v["address"] = jsToNewAddress(a.first);
                    _v["vote_num"] = toJS(a.second);
                    _arry.append(_v);
                }
                jv["electors"] = _arry;
            }
            return jv;
        }

        Json::Value toJsonV2(ex::exchange_order const& _e) {
                    Json::Value _value;
                    _value["Address"] = jsToNewAddress(_e.sender);
                    _value["Hash"] = toJS(_e.trxid);
                    _value["price"] = std::string(_e.price);
                    _value["token_amount"] = std::string(_e.token_amount);
                    _value["source_amount"] = std::string(_e.source_amount);
                    _value["create_time"] = toJS(_e.create_time);
                    std::tuple<std::string, std::string, std::string> _resultTuple = enumToString(_e.type, _e.token_type,
                                                                                      (ex::order_buy_type) 0);
                    _value["order_type"] = get<0>(_resultTuple);
                    _value["order_token_type"] = get<1>(_resultTuple);


                return _value;
        }

        Json::Value toJsonV2ByBlock(LocalisedLogEntries const &_entries) {
            vector<h256> order;
            unordered_map<h256, LocalisedLogEntries> entriesByBlock;

            for (dev::brc::LocalisedLogEntry const &e : _entries) {
                if (e.isSpecial)  // skip special log
                    continue;

                if (entriesByBlock.count(e.blockHash) == 0) {
                    entriesByBlock[e.blockHash] = LocalisedLogEntries();
                    order.push_back(e.blockHash);
                }

                entriesByBlock[e.blockHash].push_back(e);
            }

            return toJsonV2(entriesByBlock, order);
        }

        TransactionSkeleton toTransactionSkeletonV2(Json::Value const &_json) {
            TransactionSkeleton ret;
            if (!_json.isObject() || _json.empty())
                return ret;
            try {
                if (!_json["from"].empty())
                    ret.from =  jsToAddress(_json["from"].asString());
                if (!_json["to"].empty() && _json["to"].asString() != "0x" && !_json["to"].asString().empty())
                    ret.to = jsToAddress(_json["to"].asString());
                else
                    ret.creation = true;

                if (!_json["value"].empty())
                    ret.value = jsToU256(_json["value"].asString());

                if (!_json["gas"].empty())
                    ret.gas = jsToU256(_json["gas"].asString());

                if (!_json["gasPrice"].empty())
                    ret.gasPrice = jsToU256(_json["gasPrice"].asString());

                if (!_json["data"].empty())                            // brcdChain.js has preconstructed the data array
                {
                    ret.data = jsToBytes(_json["data"].asString(), OnFailed::Throw);
                }

                if (!_json["code"].empty())
                    ret.data = jsToBytes(_json["code"].asString(), OnFailed::Throw);

                if (!_json["nonce"].empty())
                    ret.nonce = jsToU256(_json["nonce"].asString());
            }
            catch (boost::exception const &) {
                throw jsonrpc::JsonRpcException("Invalid toTransactionSkeleton from json: ");
            }

            return ret;
        }

        dev::brc::LogFilter toLogFilterV2(Json::Value const &_json) {
            dev::brc::LogFilter filter;
            if (!_json.isObject() || _json.empty())
                return filter;

            // check only !empty. it should throw exceptions if input params are incorrect
            if (!_json["fromBlock"].empty())
                filter.withEarliest(jsToFixed<32>(_json["fromBlock"].asString()));
            if (!_json["toBlock"].empty())
                filter.withLatest(jsToFixed<32>(_json["toBlock"].asString()));
            if (!_json["address"].empty()) {
                if (_json["address"].isArray())
                    for (auto i : _json["address"])
                        filter.address(jsToAddress(i.asString()));
                else
                    filter.address(jsToAddress(_json["address"].asString()));
            }
            if (!_json["topics"].empty())
                for (unsigned i = 0; i < _json["topics"].size(); i++) {
                    if (_json["topics"][i].isArray()) {
                        for (auto t : _json["topics"][i])
                            if (!t.isNull())
                                filter.topic(i, jsToFixed<32>(t.asString()));
                    } else if (!_json["topics"][i].isNull())  // if it is anything else then string, it should
                        // and will fail
                        filter.topic(i, jsToFixed<32>(_json["topics"][i].asString()));
                }
            return filter;
        }

// TODO: this should be removed once we decide to remove backward compatibility with old log filters
        dev::brc::LogFilter toLogFilterV2(Json::Value const &_json,
                                        Interface const &_client)  // commented to avoid warning. Uncomment once in use @ PoC-7.
        {
            dev::brc::LogFilter filter;
            if (!_json.isObject() || _json.empty())
                return filter;

            // check only !empty. it should throw exceptions if input params are incorrect
            if (!_json["fromBlock"].empty())
                filter.withEarliest(_client.hashFromNumber(jsToBlockNumber(_json["fromBlock"].asString())));
            if (!_json["toBlock"].empty())
                filter.withLatest(_client.hashFromNumber(jsToBlockNumber(_json["toBlock"].asString())));
            if (!_json["address"].empty()) {
                if (_json["address"].isArray())
                    for (auto i : _json["address"])
                        filter.address(jsToAddress(i.asString()));
                else
                    filter.address(jsToAddress(_json["address"].asString()));
            }
            if (!_json["topics"].empty())
                for (unsigned i = 0; i < _json["topics"].size(); i++) {
                    if (_json["topics"][i].isArray()) {
                        for (auto t : _json["topics"][i])
                            if (!t.isNull())
                                filter.topic(i, jsToFixed<32>(t.asString()));
                    } else if (!_json["topics"][i].isNull())  // if it is anything else then string, it should
                        // and will fail
                        filter.topic(i, jsToFixed<32>(_json["topics"][i].asString()));
                }
            return filter;
        }

    }  // namespace brc

// ////////////////////////////////////////////////////////////////////////////////////
// rpc
// ////////////////////////////////////////////////////////////////////////////////////

//    namespace rpc {
//        h256 h256fromHex(string const &_s) {
//            try {
//                return h256(_s);
//            }
//            catch (boost::exception const &) {
//                throw jsonrpc::JsonRpcException("Invalid hex-encoded string: " + _s);
//            }
//        }
//    }  // namespace rpc

}  // namespace dev
