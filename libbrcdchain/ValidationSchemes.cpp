#include "ValidationSchemes.h"
#include <libdevcore/JsonUtils.h>
#include <string>

using namespace std;
namespace js = json_spirit;

namespace dev {
    namespace brc {
        namespace validation {
            string const c_sealEngine = "sealEngine";
            string const c_params = "params";
            string const c_genesis = "genesis";
            string const c_accounts = "accounts";
            string const c_balance = "cookies";
            string const c_brc = "brc";
			string const c_fcookie = "fcookie";
            string const c_wei = "wei";
            string const c_finney = "finney";
            string const c_author = "author";
            string const c_coinbase = "coinbase";
			string const c_currency = "currency";
            string const c_vote = "vote";
            string const c_nonce = "nonce";
            string const c_gasLimit = "gasLimit";
            string const c_timestamp = "timestamp";
            string const c_difficulty = "difficulty";
            string const c_extraData = "extraData";
            string const c_mixHash = "mixHash";
            string const c_parentHash = "parentHash";
            string const c_sign = "sign";
            string const c_precompiled = "precompiled";
            string const c_code = "code";
            string const c_storage = "storage";
            string const c_gasUsed = "gasUsed";
            string const c_codeFromFile = "codeFromFile";  ///< A file containg a code as bytes.
            string const c_shouldnotexist = "shouldnotexist";
			string const c_genesisVarlitor = "genesisVarlitor";

            string const c_minGasLimit = "minGasLimit";
            string const c_maxGasLimit = "maxGasLimit";
            string const c_gasLimitBoundDivisor = "gasLimitBoundDivisor";
            string const c_homesteadForkBlock = "homesteadForkBlock";
            string const c_daoHardforkBlock = "daoHardforkBlock";
            string const c_EIP150ForkBlock = "EIP150ForkBlock";
            string const c_EIP158ForkBlock = "EIP158ForkBlock";
            string const c_byzantiumForkBlock = "byzantiumForkBlock";
            string const c_eWASMForkBlock = "eWASMForkBlock";
            string const c_constantinopleForkBlock = "constantinopleForkBlock";
            string const c_experimentalForkBlock = "experimentalForkBlock";
            string const c_accountStartNonce = "accountStartNonce";
            string const c_maximumExtraDataSize = "maximumExtraDataSize";
            string const c_tieBreakingGas = "tieBreakingGas";
            string const c_blockReward = "blockReward";
            string const c_difficultyBoundDivisor = "difficultyBoundDivisor";
            string const c_minimumDifficulty = "minimumDifficulty";
            string const c_durationLimit = "durationLimit";
            string const c_chainID = "chainID";
            string const c_networkID = "networkID";
            string const c_allowFutureBlocks = "allowFutureBlocks";

            string const c_epochInterval = "dposEpochInterval";
            string const c_varlitorInterval = "dposVarlitorInterval";
            string const c_blockInterval = "dposBlockInterval";

            void validateConfigJson(js::mObject const &_obj) {
                requireJsonFields(_obj, "ChainParams::loadConfig",
                                  {{c_sealEngine, {{js::str_type}, JsonFieldPresence::Required}},
                                   {c_params,     {{js::obj_type}, JsonFieldPresence::Required}},
                                   {c_genesis,    {{js::obj_type}, JsonFieldPresence::Required}},
                                   {c_accounts,   {{js::obj_type}, JsonFieldPresence::Required}}});

                requireJsonFields(_obj.at(c_genesis).get_obj(), "ChainParams::loadConfig::genesis",
                                  {{c_author,     {{js::str_type}, JsonFieldPresence::Required}},
                                   {c_nonce,      {{js::str_type}, JsonFieldPresence::Required}},
                                   {c_gasLimit,   {{js::str_type}, JsonFieldPresence::Required}},
                                   {c_timestamp,  {{js::str_type}, JsonFieldPresence::Required}},
                                   {c_difficulty, {{js::str_type}, JsonFieldPresence::Required}},
                                   {c_extraData,  {{js::str_type}, JsonFieldPresence::Required}},
                                   {c_mixHash,    {{js::str_type}, JsonFieldPresence::Required}},
                                   {c_parentHash, {{js::str_type}, JsonFieldPresence::Optional}},
                                   {c_sign, {{js::str_type}, JsonFieldPresence::Optional}}
                                  });

                js::mObject const &accounts = _obj.at(c_accounts).get_obj();
                for (auto const &acc : accounts)
                    validateAccountObj(acc.second.get_obj());
            }

            void validateAccountMaskObj(js::mObject const &_obj) {
                // A map object with possibly defined fields
                requireJsonFields(_obj, "validateAccountMaskObj",
                                  {{c_storage,        {{js::obj_type}, JsonFieldPresence::Optional}},
                                   {c_balance,        {{js::str_type}, JsonFieldPresence::Optional}},
                                   {c_nonce,          {{js::str_type}, JsonFieldPresence::Optional}},
                                   {c_code,           {{js::str_type}, JsonFieldPresence::Optional}},
                                   {c_precompiled,    {{js::obj_type}, JsonFieldPresence::Optional}},
                                   {c_shouldnotexist, {{js::str_type}, JsonFieldPresence::Optional}},
                                   {c_wei,            {{js::str_type}, JsonFieldPresence::Optional}}});
            }

            void validateAccountObj(js::mObject const &_obj) {
                if (_obj.count(c_precompiled))
				{
                    // A precompiled contract
                    requireJsonFields(_obj, "validateAccountObj",
                                       {{c_precompiled, {{js::obj_type}, JsonFieldPresence::Required}},
                                       {c_wei,         {{js::str_type}, JsonFieldPresence::Optional}},
                                       {c_balance,     {{js::str_type}, JsonFieldPresence::Optional}},
										{c_brc,			{{js::str_type}, JsonFieldPresence::Optional}},
										{c_fcookie,		{{js::str_type}, JsonFieldPresence::Optional}},
										{c_genesisVarlitor, {{js::array_type}, JsonFieldPresence::Optional}},
										{c_currency, {{js::obj_type}, JsonFieldPresence::Optional}}});
                } else if (_obj.size() == 1) {
                    // A genesis account with only balance set
                    if (_obj.count(c_balance))
                        requireJsonFields(_obj, "validateAccountObj",
                                          {{c_balance, {{js::str_type}, JsonFieldPresence::Required}}});
                    else if(_obj.count(c_wei))
                        requireJsonFields(_obj, "validateAccountObj",
                                          {{c_wei, {{js::str_type}, JsonFieldPresence::Required}}});
					else if(_obj.count(c_genesisVarlitor))
						requireJsonFields(_obj, "validateAccountObj",
										  {{c_genesisVarlitor, {{js::array_type}, JsonFieldPresence::Required}}});
                    else if(_obj.count(c_vote))
						requireJsonFields(_obj, "validateAccountObj",
										  {{c_vote, {{js::obj_type}, JsonFieldPresence::Required}}});
					else
						requireJsonFields(_obj, "validateAccountObj",
										{{c_currency, {{js::obj_type}, JsonFieldPresence::Required}}});
				} else if (_obj.size() == 2) {
                    // A genesis account with only balance set
                    if (_obj.count(c_balance))
                        requireJsonFields(_obj, "validateAccountObj",
                                          {{c_balance, {{js::str_type}, JsonFieldPresence::Required}}});
                    else if(_obj.count(c_wei))
                        requireJsonFields(_obj, "validateAccountObj",
                                          {{c_wei, {{js::str_type}, JsonFieldPresence::Required}}});
					else if(_obj.count(c_genesisVarlitor))
						requireJsonFields(_obj, "validateAccountObj",
										  {{c_genesisVarlitor, {{js::array_type}, JsonFieldPresence::Required}}});
                    else if(_obj.count(c_vote))
						requireJsonFields(_obj, "validateAccountObj",
										  {{c_vote, {{js::obj_type}, JsonFieldPresence::Required}},
                                           {c_currency, {{js::obj_type}, JsonFieldPresence::Required}}});
					else if(_obj.count(c_currency))
						requireJsonFields(_obj, "validateAccountObj",
										  {{c_vote, {{js::obj_type}, JsonFieldPresence::Required}},
                                           {c_currency, {{js::obj_type}, JsonFieldPresence::Required}}});
				}
				else
				{
                    if (_obj.count(c_codeFromFile)) 
					{
                        // A standart account with external code
                        requireJsonFields(_obj, "validateAccountObj",
                                          {{c_codeFromFile, {{js::str_type}, JsonFieldPresence::Required}},
                                           {c_nonce,        {{js::str_type}, JsonFieldPresence::Required}},
                                           {c_storage,      {{js::obj_type}, JsonFieldPresence::Required}},
                                           {c_balance,      {{js::str_type}, JsonFieldPresence::Required}},
											{c_genesisVarlitor, {{js::array_type}, JsonFieldPresence::Optional}},
											{c_currency, {{js::obj_type}, JsonFieldPresence::Optional}}});
                    }
					else 
					{
                        // A standart account with all fields
                        requireJsonFields(_obj, "validateAccountObj",
                                          {{c_code,    {{js::str_type}, JsonFieldPresence::Required}},
                                           {c_nonce,   {{js::str_type}, JsonFieldPresence::Required}},
										   {c_storage, {{js::obj_type}, JsonFieldPresence::Required}},
                                           {c_balance, {{js::str_type}, JsonFieldPresence::Required}},
											{c_genesisVarlitor, {{js::array_type}, JsonFieldPresence::Optional}},
											{c_currency, {{js::obj_type}, JsonFieldPresence::Optional}}});
                    }
                }
            }
        }
    }
}
