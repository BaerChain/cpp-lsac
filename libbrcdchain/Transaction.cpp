#include "Transaction.h"
#include "Interface.h"
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
#include <libdevcore/vector_ref.h>
#include <libdevcrypto/Common.h>
#include <libbrccore/Exceptions.h>
#include <libbvm/VMFace.h>
using namespace std;
using namespace dev;
using namespace dev::brc;

#define BRC_ADDRESS_DEBUG 0

std::ostream& dev::brc::operator<<(std::ostream& _out, ExecutionResult const& _er)
{
    _out << "{" << _er.gasUsed << ", " << _er.newAddress << ", " << toHex(_er.output) << "}";
    return _out;
}

TransactionException dev::brc::toTransactionException(Exception const& _e)
{
    // Basic Transaction exceptions
    if (!!dynamic_cast<RLPException const*>(&_e))
        return TransactionException::BadRLP;
    if (!!dynamic_cast<OutOfGasIntrinsic const*>(&_e))
        return TransactionException::OutOfGasIntrinsic;
    if (!!dynamic_cast<InvalidSignature const*>(&_e))
        return TransactionException::InvalidSignature;
    // Executive exceptions
    if (!!dynamic_cast<OutOfGasBase const*>(&_e))
        return TransactionException::OutOfGasBase;
    if (!!dynamic_cast<InvalidNonce const*>(&_e))
        return TransactionException::InvalidNonce;
    if (!!dynamic_cast<NotEnoughCash const*>(&_e))
        return TransactionException::NotEnoughCash;
    if (!!dynamic_cast<NotEnoughBallot const*>(&_e))
        return TransactionException::NotEnoughBallot;
    if (!!dynamic_cast<BlockGasLimitReached const*>(&_e))
        return TransactionException::BlockGasLimitReached;
    if (!!dynamic_cast<AddressAlreadyUsed const*>(&_e))
        return TransactionException::AddressAlreadyUsed;
    // VM execution exceptions
    if (!!dynamic_cast<BadInstruction const*>(&_e))
        return TransactionException::BadInstruction;
    if (!!dynamic_cast<BadJumpDestination const*>(&_e))
        return TransactionException::BadJumpDestination;
    if (!!dynamic_cast<OutOfGas const*>(&_e))
        return TransactionException::OutOfGas;
    if (!!dynamic_cast<OutOfStack const*>(&_e))
        return TransactionException::OutOfStack;
    if (!!dynamic_cast<StackUnderflow const*>(&_e))
        return TransactionException::StackUnderflow;
    return TransactionException::Unknown;
}

std::ostream& dev::brc::operator<<(std::ostream& _out, TransactionException const& _er)
{
    switch (_er)
    {
    case TransactionException::None:
        _out << "None";
        break;
    case TransactionException::BadRLP:
        _out << "BadRLP";
        break;
    case TransactionException::InvalidFormat:
        _out << "InvalidFormat";
        break;
    case TransactionException::OutOfGasIntrinsic:
        _out << "OutOfGasIntrinsic";
        break;
    case TransactionException::InvalidSignature:
        _out << "InvalidSignature";
        break;
    case TransactionException::InvalidNonce:
        _out << "InvalidNonce";
        break;
    case TransactionException::NotEnoughCash:
        _out << "NotEnoughCash";
        break;
    case TransactionException::NotEnoughBallot:
        _out << "NotEnoughBallot";
        break;
    case TransactionException::OutOfGasBase:
        _out << "OutOfGasBase";
        break;
    case TransactionException::BlockGasLimitReached:
        _out << "BlockGasLimitReached";
        break;
    case TransactionException::BadInstruction:
        _out << "BadInstruction";
        break;
    case TransactionException::BadJumpDestination:
        _out << "BadJumpDestination";
        break;
    case TransactionException::OutOfGas:
        _out << "OutOfGas";
        break;
    case TransactionException::OutOfStack:
        _out << "OutOfStack";
        break;
    case TransactionException::StackUnderflow:
        _out << "StackUnderflow";
        break;
    default:
        _out << "Unknown";
        break;
    }
    return _out;
}

Transaction::Transaction(bytesConstRef _rlpData, CheckTransaction _checkSig)
  : TransactionBase(_rlpData, _checkSig)
{}

bytes transationTool::transferMutilSigns_operation::datasBytes() const{
    RLPStream s;
    std::vector<bytes> datas;
    for (auto const& a: m_data_ptrs){
        datas.emplace_back(a->serialize());
    }
    s.appendVector(datas);
    return s.out();
}

std::vector<bytes> transationTool::transferMutilSigns_operation::getTransactionDatabytes() const{
    std::vector<bytes> datas;
    for (auto const& a: m_data_ptrs){
        datas.emplace_back(a->serialize());
    }
    return datas;
}

bytes transationTool::transferMutilSigns_operation::serialize()  const
{
    RLPStream s(5);
    s<< m_type << m_rootAddress<< m_cookiesAddress << datasBytes();
    std::vector<bytes> sign_bs;
    for(auto const& v: m_signs){
        if (v.r && v.s) {
            RLPStream sign(3);
            sign << (u256)v.r << (u256)v.s << v.v;
            sign_bs.emplace_back(sign.out());
            cwarn << "signs:" << dev::toJS(sign.out());
        }
    }
    s.appendVector<bytes>(sign_bs);
    return s.out();
}
transationTool::transferMutilSigns_operation::transferMutilSigns_operation(bytes const& _bs)
{
    RLP rlp(_bs);
    m_type = (op_type)rlp[0].convert<uint8_t>(RLP::LaissezFaire);
    m_rootAddress = rlp[1].convert<Address>(RLP::LaissezFaire);
    m_cookiesAddress = rlp[2].convert<Address>(RLP::LaissezFaire);
    auto op_bs = rlp[3].toBytes();
    RLP r(op_bs);
    for(auto const& op: r.toVector<bytes>()){
        auto op_ptr = getOperationByRLP(op);
        m_data_ptrs.emplace_back(std::shared_ptr<operation>(op_ptr));
    }
    //m_data_ptr.reset(getOperationByRLP(op_bs));
    auto signs = rlp[4].toVector<bytes>();
    for(auto const& r:signs){
        RLP sign(r);
        auto _sign = SignatureStruct{(h256)sign[0].toInt<u256>(),(h256)sign[1].toInt<u256>(), sign[2].toInt<uint8_t>()};
        m_signs.emplace_back(_sign);
    }
}

std::vector<Address> transationTool::transferMutilSigns_operation::getSignAddress(){
    std::vector<Address> childAddrs;
    if(m_data_ptrs.empty())
        return childAddrs;
    auto data_rlp_sha3 =dev::sha3(datasBytes());
    for(auto const& k: m_signs){
        if (!k.isValid())
            BOOST_THROW_EXCEPTION(TransactionIsUnsigned());
        auto p = recover(k, data_rlp_sha3 );
        if (!p)
            BOOST_THROW_EXCEPTION(InvalidSignature());
        auto addr = right160(dev::sha3(bytesConstRef(p.data(), sizeof(p))));
        childAddrs.emplace_back(addr);
    }
    return childAddrs;
}

transationTool::operation* transationTool::getOperationByRLP(bytes const& _bs)
{
    auto type = operation::get_type(_bs);
    if(type == op_type::vote)
        return new vote_operation(_bs);
    else if(type == op_type::brcTranscation)
        return new transcation_operation(_bs);
    else if(type == op_type::cancelPendingOrder)
        return new cancelPendingorder_operation(_bs);
    else if(type == op_type::pendingOrder)
        return new pendingorder_opearaion(_bs);
    else if(type == op_type::receivingincome)
        return new receivingincome_operation(_bs);
    else if(type == op_type::transferAutoEx)
        return new transferAutoEx_operation(_bs);
    else
        BOOST_THROW_EXCEPTION(InvalidMutilTransactionType() <<errinfo_comment("invalid transaction type in mutilTrasnction"));
    return nullptr;
}

authority::PermissionsType authority::getPermissionsTypeByTransactionType(transationTool::op_type _type){
    // verify the op_type is allowed
    if(_type == transationTool::op_type::vote || _type == transationTool::op_type::brcTranscation ||
       _type == transationTool::op_type::pendingOrder || _type == transationTool::op_type::cancelPendingOrder||
       _type == transationTool::op_type::receivingincome || _type == transationTool::op_type::transferAutoEx ||
       _type == transationTool::op_type::executeContract )
    {
        return _type;
    }
    BOOST_THROW_EXCEPTION(InvalidMutilTransactionType() << errinfo_comment("transaction type:"+ std::to_string(_type) + " can't to mutilSings"));
}