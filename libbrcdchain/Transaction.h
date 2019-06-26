#pragma once



#include <libbrccore/ChainOperationParams.h>
#include <libbrccore/Common.h>
#include <libbrccore/TransactionBase.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <boost/preprocessor/seq.hpp>
#include <libdevcrypto/Common.h>

//#include "brc/types.hpp"
#include <brc/types.hpp>


namespace dev
{
    namespace brc
    {


        enum class TransactionException
        {
            None = 0,
            Unknown,
            BadRLP,
            InvalidFormat,
            OutOfGasIntrinsic,  ///< Too little gas to pay for the base transaction cost.
            InvalidSignature,
            InvalidNonce,
            InvalidGasPrice,
            NotEnoughCash,
            OutOfGasBase,  ///< Too little gas to pay for the base transaction cost.
            BlockGasLimitReached,
            BadInstruction,
            BadJumpDestination,
            OutOfGas,    ///< Ran out of gas executing code of the transaction.
            OutOfStack,  ///< Ran out of stack executing code of the transaction.
            StackUnderflow,
            RevertInstruction,
            InvalidZeroSignatureFormat,
            AddressAlreadyUsed,
            NotEnoughBallot,
            VerifyVoteField,
            VerifyPendingOrderFiled,
            CancelPendingOrderFiled,
            BadSystemAddress,
            BadVoteParamter,
            BadBRCTransactionParamter,
            BrcTranscationField,
            DefaultError
        };

        namespace transationTool
        {
#define SERIALIZE_MACRO(r, data, elem) data.append(elem);
#define OPERATION_SERIALIZE(MEMBERS)                             \
    virtual bytes serialize()  const                                  \
    {                                                            \
        RLPStream stream(BOOST_PP_SEQ_SIZE(MEMBERS));            \
        BOOST_PP_SEQ_FOR_EACH(SERIALIZE_MACRO, stream, MEMBERS); \
        return stream.out();                                     \
    }
#define UNSERIALIZE_MACRO(r, data, i, elem) \
    elem = data[i].convert<decltype(elem)>(RLP::LaissezFaire);
#define OPERATION_UNSERIALIZE(CALSS, MEMBERS)                    \
    CALSS(const bytes& Data)                                     \
    {                                                            \
        RLP rlp(Data);                                           \
        BOOST_PP_SEQ_FOR_EACH_I(UNSERIALIZE_MACRO, rlp, MEMBERS) \
    }

            enum op_type : uint8_t
            {
                null = 0,
                vote = 1,
                brcTranscation = 2,
                pendingOrder = 3,
                cancelPendingOrder = 4,
                deployContract =5,
                executeContract =6,
                controlAccount =7,
            };

            static std::map<op_type, u256> c_add_value = {
                    {null, 0},
                    {vote, 5000},
                    {brcTranscation, 0},
                    {pendingOrder, 10000},
                    {cancelPendingOrder, 2000},
                    {deployContract, 0},
                    {executeContract, 0},
					{controlAccount, 0}

            };

            struct operation
            {
                virtual ~operation() {}
                static op_type get_type(const bytes& data)
                {
                    try
                    {
                        RLP rlp(data);
                        return (op_type)rlp[0].toInt<uint8_t>();
                    }
                    catch (const boost::exception& e)
                    {
                        // TODO throw exception or log message
                        std::cout << "exception get type" << std::endl;
                        return null;
                    }
                }

                virtual bytes serialize() const{ return bytes(); }
            };
            struct vote_operation : public operation
            {
                Address m_from;
                Address m_to;
                size_t m_vote_numbers = 0;
                uint8_t m_type = null;
                uint8_t m_vote_type = 0;
                vote_operation(
                        op_type type, const Address& from, const Address& to, uint8_t vote_type, size_t vote_num)
                        : m_type(type), m_from(from), m_to(to), m_vote_type(vote_type), m_vote_numbers(vote_num){
                }
                /// unserialize from data
                /// \param Data
                OPERATION_UNSERIALIZE(vote_operation, (m_type)(m_from)(m_to)(m_vote_type)(m_vote_numbers))

                /// bytes serialize this struct
                /// \return  bytes
                OPERATION_SERIALIZE((m_type)(m_from)(m_to)(m_vote_type)(m_vote_numbers))

                virtual ~vote_operation(){ }
            };

            struct transcation_operation : public operation
            {
                uint8_t m_type = null;
                Address m_from;
                Address m_to;
                uint8_t m_Transcation_type = 0;
                u256 m_Transcation_numbers = 0;
                transcation_operation(op_type type, const Address& from, const Address& to,
                                      uint8_t transcation_type, u256 transcation_num)
                        : m_type(type),
                          m_from(from),
                          m_to(to),
                          m_Transcation_type(transcation_type),
                          m_Transcation_numbers(transcation_num){
                }
                /// unserialize from data
                /// \param Data
                OPERATION_UNSERIALIZE(
                        transcation_operation, (m_type)(m_from)(m_to)(m_Transcation_type)(m_Transcation_numbers))

                /// bytes serialize this struct
                /// \return  bytes
                OPERATION_SERIALIZE((m_type)(m_from)(m_to)(m_Transcation_type)(m_Transcation_numbers))
            };

            struct pendingorder_opearaion : public operation
            {
                uint8_t m_type = null;
                Address m_from;
                u256 m_Pendingorder_num = 0;
                u256 m_Pendingorder_price = 0;
                ex::order_type m_Pendingorder_type = ex::order_type::null_type;
                ex::order_token_type m_Pendingorder_Token_type = ex::order_token_type::BRC;
                ex::order_buy_type m_Pendingorder_buy_type = ex::order_buy_type::all_price;
                pendingorder_opearaion(){ }
                pendingorder_opearaion(
                        op_type type, const Address& from, ex::order_type pendingorder_type, ex::order_token_type _pendingorder_token_type,
                        ex::order_buy_type _pendingorder_buy_type, u256 pendingorder_num, u256 pendingorder_price)
                        : m_type(type),
                          m_from(from),
                          m_Pendingorder_type(pendingorder_type),
                          m_Pendingorder_Token_type(_pendingorder_token_type),
                          m_Pendingorder_buy_type(_pendingorder_buy_type),
                          m_Pendingorder_num(pendingorder_num),
                          m_Pendingorder_price(pendingorder_price){
                }

                //	OPERATION_UNSERIALIZE(pendingorder_opearaion, (m_type)(m_from)(m_Pendingorder_type)(m_Pendingorder_Token_type)(m_Pendingorder_buy_type)(m_Pendingorder_num)(m_Pendingorder_price))
                pendingorder_opearaion(const bytes& Data){
                    RLP rlp(Data);
                    m_type = rlp[0].convert<uint8_t>(RLP::LaissezFaire);
                    m_from = rlp[1].convert<Address>(RLP::LaissezFaire);
                    m_Pendingorder_type = (ex::order_type)rlp[2].convert<uint8_t>(RLP::LaissezFaire);
                    m_Pendingorder_Token_type = (ex::order_token_type)rlp[3].convert<uint8_t>(RLP::LaissezFaire);
                    m_Pendingorder_buy_type = (ex::order_buy_type)rlp[4].convert<uint8_t>(RLP::LaissezFaire);
                    m_Pendingorder_num = rlp[5].convert<u256>(RLP::LaissezFaire);
                    m_Pendingorder_price = rlp[6].convert<u256>(RLP::LaissezFaire);
                }


                //	OPERATION_SERIALIZE((m_type)(m_from)(m_Pendingorder_type)(m_Pendingorder_Token_type)(m_Pendingorder_buy_type)(m_Pendingorder_num)(m_Pendingorder_price))
                virtual bytes serialize()  const{
                    RLPStream stream(7);
                    stream.append((uint8_t)m_type);
                    stream.append(m_from);
                    stream.append((uint8_t)m_Pendingorder_type);
                    stream.append((uint8_t)m_Pendingorder_Token_type);
                    stream.append((uint8_t)m_Pendingorder_buy_type);
                    stream.append(m_Pendingorder_num);
                    stream.append(m_Pendingorder_price);
                    return stream.out();
                }
            };

            struct cancelPendingorder_operation : public operation
            {
                h256 m_hash;
                uint8_t m_type = 4;
                uint8_t m_cancelType = 3;

                cancelPendingorder_operation(){}
                cancelPendingorder_operation(uint8_t type, uint8_t cancel_type,h256 _hash):m_type(type), m_cancelType(cancel_type), m_hash(_hash)
                {}

                OPERATION_UNSERIALIZE(cancelPendingorder_operation, (m_type)(m_cancelType)(m_hash))

                OPERATION_SERIALIZE((m_type)(m_cancelType)(m_hash))
            };

            struct contract_operation : public operation
            {
                op_type m_type;
                bytes m_date;
                contract_operation(){}
                contract_operation(op_type _type, bytes _d):m_type(_type) { m_date = _d; }
                OPERATION_UNSERIALIZE(contract_operation, (m_date))
                OPERATION_SERIALIZE((m_date))
            };

			struct control_acconut_operation :public operation
			{
				uint8_t m_type;
				Public m_conotrol_addr;         // change pubilc_key 
				size_t m_weight;        // weight
				long m_authority;       // authority

				control_acconut_operation(op_type type, const Public& conotrol_addr, size_t weight, long authority)
					: m_type(type), m_conotrol_addr(conotrol_addr), m_weight(weight), m_authority(authority){
				}
				/// unserialize from data
				/// \param Data
				OPERATION_UNSERIALIZE(control_acconut_operation, (m_type)(m_conotrol_addr)(m_weight)(m_authority))
				/// bytes serialize this struct
				/// \return  bytes
			    OPERATION_SERIALIZE((m_type)(m_conotrol_addr)(m_weight)(m_authority))
				virtual ~control_acconut_operation(){ }
			};

        }  // namespace transationTool

        enum class CodeDeposit
        {
            None = 0,
            Failed,
            Success
        };

        struct VMException;

        TransactionException toTransactionException(Exception const& _e);
        std::ostream& operator<<(std::ostream& _out, TransactionException const& _er);

/// Description of the result of executing a transaction.
        struct ExecutionResult
        {
            u256 gasUsed = 0;
            TransactionException excepted = TransactionException::Unknown;
            Address newAddress;
            bytes output;
            CodeDeposit codeDeposit =
                    CodeDeposit::None;  ///< Failed if an attempted deposit failed due to lack of gas.
            u256 gasRefunded = 0;
            unsigned depositSize = 0;  ///< Amount of code of the creation's attempted deposit.
            u256 gasForDeposit;        ///< Amount of gas remaining for the code deposit phase.
        };

        std::ostream& operator<<(std::ostream& _out, ExecutionResult const& _er);

/// Encodes a transaction, ready to be exported to or freshly imported from RLP.
        class Transaction : public TransactionBase
        {
        public:
            /// Constructs a null transaction.
            Transaction() {}

            /// Constructs from a transaction skeleton & optional secret.
            Transaction(TransactionSkeleton const& _ts, Secret const& _s = Secret())
                    : TransactionBase(_ts, _s)
            {}

            /// 创建dpos相关的交易
            // Transaction(TransactionSkeleton const& _ts, Secret const& _s, u256 _flag);

            /// Constructs a signed message-call transaction.
            Transaction(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, Address const& _dest,
                        bytes const& _data, u256 const& _nonce, Secret const& _secret)
                    : TransactionBase(_value, _gasPrice, _gas, _dest, _data, _nonce, _secret)
            {}

            /// Constructs a signed contract-creation transaction.
            Transaction(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, bytes const& _data,
                        u256 const& _nonce, Secret const& _secret)
                    : TransactionBase(_value, _gasPrice, _gas, _data, _nonce, _secret)
            {}

            /// Constructs an unsigned message-call transaction.
            Transaction(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, Address const& _dest,
                        bytes const& _data, u256 const& _nonce = Invalid256)
                    : TransactionBase(_value, _gasPrice, _gas, _dest, _data, _nonce)
            {}

            /// Constructs an unsigned contract-creation transaction.
            Transaction(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, bytes const& _data,
                        u256 const& _nonce = Invalid256)
                    : TransactionBase(_value, _gasPrice, _gas, _data, _nonce)
            {}

            /// Constructs a transaction from the given RLP.
            explicit Transaction(bytesConstRef _rlp, CheckTransaction _checkSig);

            /// Constructs a transaction from the given RLP.
            explicit Transaction(bytes const& _rlp, CheckTransaction _checkSig)
                    : Transaction(&_rlp, _checkSig)
            {}
        };

/// Nice name for vector of Transaction.
        using Transactions = std::vector<Transaction>;

        class LocalisedTransaction : public Transaction
        {
        public:
            LocalisedTransaction(Transaction const& _t, h256 const& _blockHash, unsigned _transactionIndex,
                                 BlockNumber _blockNumber = 0)
                    : Transaction(_t),
                      m_blockHash(_blockHash),
                      m_transactionIndex(_transactionIndex),
                      m_blockNumber(_blockNumber)
            {}

            h256 const& blockHash() const { return m_blockHash; }
            unsigned transactionIndex() const { return m_transactionIndex; }
            BlockNumber blockNumber() const { return m_blockNumber; }

        private:
            h256 m_blockHash;
            unsigned m_transactionIndex;
            BlockNumber m_blockNumber;
        };

#define BALLOTPRICE 500000000
    }  // namespace brc
}  // namespace dev
