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
bytes transationTool::transferMutilSigns_operation::serialize()  const
{
    RLPStream s(4);
    s<< m_type << m_rootAddress << m_data_ptr->serialize();
    std::vector<bytes> sign_bs;
    cwarn << "*******sign size:"<<m_signs.size();
    for(auto const& v: m_signs){
        if (!v.r && !v.s) {
            RLPStream sign(3);
            sign << v.r << v.s << v.v;
            sign_bs.emplace_back(sign.out());
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
    auto op_bs = rlp[2].convert<bytes>(RLP::LaissezFaire);
    //m_data_ptr = std::make_shared<operation>(getOperationByRLP(op_bs));
    m_data_ptr.reset(getOperationByRLP(op_bs));
    auto signs = rlp[3].toVector<bytes>();
    for(auto const& r:signs){
        RLP sign(r);
        auto _sign = SignatureStruct{sign[0].toInt<u256>(),sign[1].toInt<u256>(), sign[2].toInt<uint8_t>()};
        m_signs.emplace_back(_sign);
    }

    //    if(type == op_type::vote)
//        m_data_ptr = std::make_shared<vote_operation>(new vote_operation(op_bs));
//    else if(type == op_type::brcTranscation)
//        m_data_ptr = std::make_shared<transcation_operation>(new transcation_operation(op_bs));
//    else if(type == op_type::cancelPendingOrder)
//        m_data_ptr = std::make_shared<cancelPendingorder_operation>(new cancelPendingorder_operation(op_bs));
//    else if(type == op_type::pendingOrder)
//        m_data_ptr = std::make_shared<pendingorder_opearaion>(new pendingorder_opearaion(op_bs));
//    else if(type == op_type::receivingincome)
//        m_data_ptr = std::make_shared<receivingincome_operation>(new receivingincome_operation(op_bs));
//    else if(type == op_type::transferAutoEx)
//        m_data_ptr = std::make_shared<transferAutoEx_operation>(new transferAutoEx_operation(op_bs));
////    else if(type == op_type::transferAccountControl){
////    }
//    else{
//    }


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
//    else if(type == op_type::transferAccountControl){
//    }
    return nullptr;
}