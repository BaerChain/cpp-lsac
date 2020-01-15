#include "Executive.h"

#include "Block.h"
#include "BlockChain.h"
#include "ExtVM.h"
#include "Interface.h"
#include "State.h"

#include <libdevcore/CommonIO.h>
#include <libbrccore/CommonJS.h>
#include <libbvm/LegacyVM.h>
#include <libbvm/VMFactory.h>

#include <json/json.h>
#include <boost/timer.hpp>

#include <numeric>

using namespace std;
using namespace dev;
using namespace dev::brc;


namespace
{
std::string dumpStackAndMemory(LegacyVM const& _vm)
{
    ostringstream o;
    o << "\n    STACK\n";
    for (auto i : _vm.stack())
        o << (h256)i << "\n";
    o << "    MEMORY\n"
      << ((_vm.memory().size() > 1000) ? " mem size greater than 1000 bytes " :
                                         memDump(_vm.memory()));
    return o.str();
};

std::string dumpStorage(ExtVM const& _ext)
{
    ostringstream o;
    o << "    STORAGE\n";
    for (auto const& i : _ext.state().storage(_ext.myAddress))
        o << showbase << hex << i.second.first << ": " << i.second.second << "\n";
    return o.str();
};


}  // namespace

StandardTrace::StandardTrace() : m_trace(Json::arrayValue) {}

bool changesMemory(Instruction _inst)
{
    return _inst == Instruction::MSTORE || _inst == Instruction::MSTORE8 ||
           _inst == Instruction::MLOAD || _inst == Instruction::CREATE ||
           _inst == Instruction::CALL || _inst == Instruction::CALLCODE ||
           _inst == Instruction::SHA3 || _inst == Instruction::CALLDATACOPY ||
           _inst == Instruction::CODECOPY || _inst == Instruction::EXTCODECOPY ||
           _inst == Instruction::DELEGATECALL;
}

bool changesStorage(Instruction _inst)
{
    return _inst == Instruction::SSTORE;
}

void StandardTrace::operator()(uint64_t _steps, uint64_t PC, Instruction inst, bigint newMemSize,
    bigint gasCost, bigint gas, VMFace const* _vm, ExtVMFace const* voidExt)
{
    (void)_steps;

    ExtVM const& ext = dynamic_cast<ExtVM const&>(*voidExt);
    auto vm = dynamic_cast<LegacyVM const*>(_vm);

    Json::Value r(Json::objectValue);

    Json::Value stack(Json::arrayValue);
    if (vm && !m_options.disableStack)
    {
        // Try extracting information about the stack from the VM is supported.
        for (auto const& i : vm->stack())
            stack.append(toCompactHexPrefixed(i, 1));
        r["stack"] = stack;
    }

    bool newContext = false;
    Instruction lastInst = Instruction::STOP;

    if (m_lastInst.size() == ext.depth)
    {
        // starting a new context
        assert(m_lastInst.size() == ext.depth);
        m_lastInst.push_back(inst);
        newContext = true;
    }
    else if (m_lastInst.size() == ext.depth + 2)
    {
        m_lastInst.pop_back();
        lastInst = m_lastInst.back();
    }
    else if (m_lastInst.size() == ext.depth + 1)
    {
        // continuing in previous context
        lastInst = m_lastInst.back();
        m_lastInst.back() = inst;
    }
    else
    {
        cwarn << "GAA!!! Tracing VM and more than one new/deleted stack frame between steps!";
        cwarn << "Attmepting naive recovery...";
        m_lastInst.resize(ext.depth + 1);
    }

    Json::Value memJson(Json::arrayValue);
    if (vm && !m_options.disableMemory && (changesMemory(lastInst) || newContext))
    {
        for (unsigned i = 0; i < vm->memory().size(); i += 32)
        {
            bytesConstRef memRef(vm->memory().data() + i, 32);
            memJson.append(toHex(memRef));
        }
        r["memory"] = memJson;
    }

    if (!m_options.disableStorage &&
        (m_options.fullStorage || changesStorage(lastInst) || newContext))
    {
        Json::Value storage(Json::objectValue);
        for (auto const& i : ext.state().storage(ext.myAddress))
            storage[toCompactHexPrefixed(i.second.first, 1)] =
                toCompactHexPrefixed(i.second.second, 1);
        r["storage"] = storage;
    }

    if (m_showMnemonics)
        r["op"] = instructionInfo(inst).name;
    r["pc"] = toString(PC);
    r["gas"] = toString(gas);
    r["gasCost"] = toString(gasCost);
    r["depth"] = toString(ext.depth);
    if (!!newMemSize)
        r["memexpand"] = toString(newMemSize);

    m_trace.append(r);
}

std::string StandardTrace::styledJson() const
{
    return Json::StyledWriter().write(m_trace);
}

string StandardTrace::multilineTrace() const
{
    if (m_trace.empty())
        return {};

    // Each opcode trace on a separate line
    return std::accumulate(std::next(m_trace.begin()), m_trace.end(),
        Json::FastWriter().write(m_trace[0]),
        [](std::string a, Json::Value b) { return a + Json::FastWriter().write(b); });
}

Executive::Executive(Block& _s, BlockChain const& _bc, unsigned _level)
  : m_vote(_s.mutableState()),
    m_exdb(_s.mutableState().exdb()),
    m_brctranscation(_s.mutableState()),
    m_s(_s.mutableState()),
    m_envInfo(_s.info(), _bc.lastBlockHashes(), 0),
    m_depth(_level),
    m_sealEngine(*_bc.sealEngine())
{}

Executive::Executive(Block& _s, LastBlockHashesFace const& _lh, unsigned _level)
  : m_vote(_s.mutableState()),
    m_exdb(_s.mutableState().exdb()),
    m_brctranscation(_s.mutableState()),
    m_s(_s.mutableState()),
    m_envInfo(_s.info(), _lh, 0),
    m_depth(_level),
    m_sealEngine(*_s.sealEngine())
{}

Executive::Executive(
    State& io_s, Block const& _block, unsigned _txIndex, BlockChain const& _bc, unsigned _level)
  : m_vote(io_s),
    m_exdb(io_s.exdb()),
    m_brctranscation(io_s),
    m_s(createIntermediateState(io_s, _block, _txIndex, _bc)),
    m_envInfo(_block.info(), _bc.lastBlockHashes(),
        _txIndex ? _block.receipt(_txIndex - 1).cumulativeGasUsed() : 0),
    m_depth(_level),
    m_sealEngine(*_bc.sealEngine())
{}

u256 Executive::gasUsed() const
{
	return m_t.gas() - m_gas;
}

void Executive::accrueSubState(SubState& _parentContext)
{
    if (m_ext)
        _parentContext += m_ext->sub;
}

void Executive::initialize(Transaction const& _transaction, transationTool::initializeEnum _enum)
{
    m_t = _transaction;
    m_baseGasRequired = m_t.baseGasRequired(m_sealEngine.brcSchedule(m_envInfo.number()));
	m_addCostValue = 0;
	try
    {
        m_sealEngine.verifyTransaction( ImportRequirements::Everything, m_t, m_envInfo.header(), m_envInfo.gasUsed());
    }
    catch (Exception const& ex)
    {
        m_excepted = toTransactionException(ex);
		std::string ex_str ="";
		if(auto *_error = boost::get_error_info<errinfo_comment>(ex))
			ex_str = std::string(*_error);
        if(m_excepted == TransactionException::InvalidSignature)
		    BOOST_THROW_EXCEPTION(InvalidSignature() << errinfo_comment(ex_str));
    }

    if (!m_t.hasZeroSignature())
    {
        // Avoid invalid transactions.
        u256 nonceReq;
        try
        {
            nonceReq = m_s.getNonce(m_t.sender());
        }
        catch (InvalidSignature const&)
        {
            LOG(m_execLogger) << "Invalid Signature";
            m_excepted = TransactionException::InvalidSignature;
            throw;
        }


        if((m_envInfo.number() <= 1740000 && m_envInfo.header().chain_id() == 0xb)
            || (m_envInfo.number() <= 1200157 && m_envInfo.header().chain_id() == 0x1))
        {
             if (m_t.nonce() < nonceReq)
            {
                cdebug << "Sender: " << m_t.sender().hex() << " Invalid Nonce: Require "
                       << nonceReq << " Got " << m_t.nonce();
                m_excepted = TransactionException::InvalidNonce;
                BOOST_THROW_EXCEPTION(
                        InvalidNonce() << RequirementError((bigint)nonceReq, (bigint)m_t.nonce())
                                       << errinfo_comment(std::string("the sender Nonce error")));
            }
        }else{
            if(_enum == transationTool::initializeEnum::rpcinitialize)
            {
                if (m_t.nonce() < nonceReq)
                {
                    cdebug << "Sender: " << m_t.sender().hex() << " Invalid Nonce: Require "
                       << nonceReq << " Got " << m_t.nonce();
                    m_excepted = TransactionException::InvalidNonce;
                    BOOST_THROW_EXCEPTION(
                        InvalidNonce() << RequirementError((bigint)nonceReq, (bigint)m_t.nonce())
                                       << errinfo_comment(std::string("the sender Nonce error")));
                }
            }else{
                if (m_t.nonce() != nonceReq)
                    {
                        cdebug << "Sender: " << m_t.sender().hex() << " Invalid Nonce: Require "
                        << nonceReq << " Got " << m_t.nonce();
                        m_excepted = TransactionException::InvalidNonce;
                        BOOST_THROW_EXCEPTION(
                            InvalidNonce() << RequirementError((bigint)nonceReq, (bigint)m_t.nonce())
                                       << errinfo_comment(std::string("the sender Nonce error")));
                }
            }

        }


        //check gasPrice the must bigger c_min_price
        cerror << "now gasPrice is :" << m_sealEngine.chainParams().m_minGasPrice;
        cerror << "enum is " << _enum;
        cerror << "transaction gas is " << m_t.gasPrice();
        if(m_envInfo.number() > config::gasPriceHeight())
        {
            if(m_t.gasPrice() < m_s.getAveragegasPrice() && _enum == transationTool::initializeEnum::rpcinitialize){
                cdebug << "Sender: " << m_t.sender().hex() << "rpcinitialize Invalid gasPrice: Require >"
				    << m_s.getAveragegasPrice() << " Got " << m_t.gasPrice();
			    m_excepted = TransactionException::InvalidGasPrice;
			    BOOST_THROW_EXCEPTION(InvalidGasPrice()<< errinfo_comment(std::string("the transaction gasPrice is lower must bigger " + toString(m_sealEngine.chainParams().m_minGasPrice))));
		    }else if(m_t.gasPrice() < m_s.getAveragegasPrice() && _enum == transationTool::initializeEnum::executeinitialize)
            {
                cdebug << "Sender: " << m_t.sender().hex() << "executeinitialize Invalid gasPrice: Require >"
				    << m_s.getAveragegasPrice() << " Got " << m_t.gasPrice();
			    m_excepted = TransactionException::InvalidGasPrice;
			    BOOST_THROW_EXCEPTION(InvalidGasPrice()<< errinfo_comment(std::string("the transaction gasPrice is lower must bigger " + toString(m_s.getAveragegasPrice()))));
            }
        }else{
            if(m_t.gasPrice() < m_sealEngine.chainParams().m_minGasPrice && _enum == transationTool::initializeEnum::rpcinitialize){
                cdebug << "Sender: " << m_t.sender().hex() << "rpcinitialize Invalid gasPrice: Require >"
				    << m_sealEngine.chainParams().m_minGasPrice << " Got " << m_t.gasPrice();
			    m_excepted = TransactionException::InvalidGasPrice;
			    BOOST_THROW_EXCEPTION(InvalidGasPrice()<< errinfo_comment(std::string("the transaction gasPrice is lower must bigger " + toString(m_sealEngine.chainParams().m_minGasPrice))));
		    }else if(m_t.gasPrice() < c_min_price && _enum == transationTool::initializeEnum::executeinitialize)
            {
                cdebug << "Sender: " << m_t.sender().hex() << "executeinitialize Invalid gasPrice: Require >"
				    << c_min_price << " Got " << m_t.gasPrice();
			    m_excepted = TransactionException::InvalidGasPrice;
			    BOOST_THROW_EXCEPTION(InvalidGasPrice()<< errinfo_comment(std::string("the transaction gasPrice is lower must bigger " + toString(c_min_price))));
            }
        }
        // Avoid unaffordable transactions.
        bigint gasCost = (bigint)m_t.gas() * m_t.gasPrice();
		u256 total_brc = 0;
        if (!m_t.isVoteTranction())
        {
			bigint totalCost = gasCost ;

            if ( m_s.balance(m_t.sender()) < totalCost || m_s.BRC(m_t.sender()) < m_t.value() || m_t.gas() < m_baseGasRequired)
            {
                LOG(m_execLogger) << "Not enough brc: Require > " << "totalCost " << " = "
					              << totalCost << "  m_t.gas() = " << m_t.gas()
					              << " * m_t.gasPrice()" << m_t.gasPrice() << " + "
                                  << m_t.value() << " Got" << m_s.BRC(m_t.sender())
                                  << " for sender: " << m_t.sender();
                m_excepted = TransactionException::NotEnoughCash;
				std::string ex_info = "not enough BRC or Cookie to execute tarnsaction will cost:"+ toString(totalCost);
				BOOST_THROW_EXCEPTION(ExecutiveFailed() << RequirementError((bigint)m_t.value(),(bigint)m_s.BRC(m_t.sender()))
                                                      << errinfo_comment(ex_info));
            }
        }
        else
        {
            RLP _r(m_t.data());
            std::vector<bytes> _ops = _r.toVector<bytes>();
			if(_ops.empty())
			{
				LOG(m_execLogger)<< "m_t.sender:" << m_t.sender() << " * "<< " to:" << m_t.receiveAddress();
				m_excepted = TransactionException::BadRLP;
				std::string ex_info = "badRLP the data is empty...";
				BOOST_THROW_EXCEPTION( BadRLP()<< errinfo_comment(ex_info));
			}
			bigint totalCost = m_t.gas()* m_t.gasPrice();
			m_addCostValue = 0;
			m_batch_params.clear();
            for (auto val : _ops) {
                transationTool::op_type _type = transationTool::operation::get_type(val);
                if (m_batch_params.size() == 0) {
                    totalCost += transationTool::c_add_value[_type] * m_t.gasPrice();
                    m_addCostValue += transationTool::c_add_value[_type] * m_t.gasPrice();
                }
                bool is_verfy_cost = true;

                if (m_batch_params._type != _type && m_batch_params.size() > 0) {
                    cwarn << "There cannot be multiple types of transactions in bulk transactions";
                    BOOST_THROW_EXCEPTION(InvalidFunction() << errinfo_comment(
                            std::string("There cannot be multiple types of transactions in bulk transactions")));
                }
                /*if(_type == transationTool::vote)
				{
                    // now is closed and will open in future
					cwarn << " this function is closed type:" << _type;
					std::string ex_info = "This function is suspended type:" + toString(_type);
					BOOST_THROW_EXCEPTION(InvalidFunction() << errinfo_comment(ex_info));
				}*/
                if (_type != transationTool::brcTranscation && _ops.size() > 1) {
                    BOOST_THROW_EXCEPTION(InvalidFunction() << errinfo_comment(
                            "Only transfer transactions can be batch operated"));
                }

                if (_type == transationTool::brcTranscation && _ops.size() > 50)
                {
                    BOOST_THROW_EXCEPTION(InvalidFunction() << errinfo_comment(
                            "The number of bulk transfers cannot exceed 50"));
                }


				m_batch_params._type = _type;
                switch (_type)
                {
                case transationTool::vote:
                {
                    transationTool::vote_operation _vote_op = transationTool::vote_operation(val);
					m_batch_params._operation.push_back(std::make_shared<transationTool::vote_operation>(_vote_op));
                }
                break;
                case transationTool::brcTranscation:
                {
                    transationTool::transcation_operation _transcation_op = transationTool::transcation_operation(val);
					try {
						total_brc = _transcation_op.m_Transcation_numbers;
						m_brctranscation.verifyTranscation(m_t.sender(), _transcation_op.m_to, (size_t)_transcation_op.m_Transcation_type,total_brc);
					}
					catch(Exception &ex)
					{
						LOG(m_execLogger)
							<< "transcation field > "
							<< "m_t.sender:" << m_t.sender() << " * "
							<< " to:" << _transcation_op.m_to
							<< " transcation_type:" << _transcation_op.m_Transcation_type
							<< " transcation_num:" << _transcation_op.m_Transcation_numbers
						    << ex.what();
						m_excepted = TransactionException::BrcTranscationField;
                        cerror << "error is " << *boost::get_error_info<errinfo_comment>(ex);
                        cerror << "number is " << m_envInfo.number();
						BOOST_THROW_EXCEPTION(BrcTranscationField() << errinfo_comment(*boost::get_error_info<errinfo_comment>(ex)));
					}
					m_batch_params._operation.push_back(std::make_shared<transationTool::transcation_operation>(_transcation_op));
                }
                break;
                case transationTool::pendingOrder:
                {
					if(m_batch_params.size() > 0)
						BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment("This peding_order is not batch !"));
                    transationTool::pendingorder_opearaion _pengdingorder_op = transationTool::pendingorder_opearaion(val);
                    if(_pengdingorder_op.m_Pendingorder_buy_type == ex::order_buy_type::all_price &&
                            _pengdingorder_op.m_Pendingorder_type == ex::order_type::buy &&
                            _pengdingorder_op.m_Pendingorder_Token_type == ex::order_token_type::FUEL &&
                            m_s.balance(m_t.sender()) < totalCost)
                     {
                         m_pendingorderStatus = true;
						 is_verfy_cost = false;
                     }
					m_batch_params._operation.push_back(std::make_shared<transationTool::pendingorder_opearaion>(_pengdingorder_op));
                }
                break;
				case transationTool::cancelPendingOrder:
                {
					transationTool::cancelPendingorder_operation  _cancel_op = transationTool::cancelPendingorder_operation(val);
					m_batch_params._operation.push_back(std::make_shared<transationTool::cancelPendingorder_operation>(_cancel_op));
                }
                break;
                case transationTool::changeMiner:
                {
                    transationTool::changeMiner_operation _changeMiner_op = transationTool::changeMiner_operation(val);
					m_batch_params._operation.push_back(std::make_shared<transationTool::changeMiner_operation>(_changeMiner_op));
                }
                break;
				case transationTool::receivingincome:
                {
                    transationTool::receivingincome_operation _receiving_op = transationTool::receivingincome_operation(val);
                    m_batch_params._operation.push_back(std::make_shared<transationTool::receivingincome_operation>(_receiving_op));

                }
                break;
				case transationTool::transferAutoEx:
                {
                    transationTool::transferAutoEx_operation _autoEx_op = transationTool::transferAutoEx_operation(val);
                    m_batch_params._operation.push_back(std::make_shared<transationTool::transferAutoEx_operation>(_autoEx_op));
                }
                break;
                case transationTool::modifyMinerGasPrice:
                {
                    transationTool::modifyMinerGasPrice_operation _gasprice_op = transationTool::modifyMinerGasPrice_operation(val);
                    m_batch_params._operation.push_back(std::make_shared<transationTool::modifyMinerGasPrice_operation>(_gasprice_op));
                }
                break;
                default:
					m_excepted = TransactionException::DefaultError;
					BOOST_THROW_EXCEPTION(
						DefaultError()
						<< errinfo_comment(m_t.sender().hex()));
                    break;
                }

				if(is_verfy_cost && m_s.balance(m_t.sender()) < totalCost && _type != transationTool::transferAutoEx){
					LOG(m_execLogger) << "Not enough cash: Require > " << totalCost << " = " << m_t.gas()
						<< " * " << m_t.gasPrice() << " + " << m_t.value() << " Got"
						<< m_s.balance(m_t.sender()) << " for sender: " << m_t.sender();
					m_excepted = TransactionException::NotEnoughCash;
					std::string ex_info = "not enough Cookie to execute tarnsaction will cost:" + toString(totalCost);
					BOOST_THROW_EXCEPTION(ExecutiveFailed() << RequirementError(totalCost, (bigint)m_s.balance(m_t.sender()))<< errinfo_comment(ex_info));
				}
            }


			if(totalCost < m_t.gasPrice()* m_baseGasRequired + m_addCostValue )
			{
				m_excepted = TransactionException::NotEnoughCash;
				std::string ex_info = "not enough require cookie to execute tarnsaction will cost:" + toString(totalCost);
				BOOST_THROW_EXCEPTION(ExecutiveFailed() << errinfo_comment(ex_info));
			}
		    m_totalGas =(u256) totalCost;
			//

			try{
				if(m_batch_params._type == transationTool::op_type::vote)
					m_vote.verifyVote(m_t.sender(), m_envInfo, m_batch_params._operation);
				else if(m_batch_params._type == transationTool::op_type::pendingOrder)
					m_brctranscation.verifyPendingOrders(m_t.sender(), (u256)totalCost, m_s.exdb(), m_envInfo.timestamp(), m_baseGasRequired * m_t.gasPrice(), m_t.sha3(), m_batch_params._operation);
				else if(m_batch_params._type == transationTool::op_type::cancelPendingOrder)
					m_brctranscation.verifyCancelPendingOrders(m_s.exdb(), m_t.sender(), m_batch_params._operation);
				else if(m_batch_params._type == transationTool::op_type::receivingincome)
                    m_brctranscation.verifyreceivingincomeChanegeMiner(m_t.sender(), m_batch_params._operation,transationTool::dividendcycle::blocknum, m_envInfo, m_vote);
                else if(m_batch_params._type == transationTool::op_type::changeMiner)
                    m_s.verifyChangeMiner(m_t.sender(), m_envInfo, m_batch_params._operation);
			    else if(m_batch_params._type == transationTool::op_type::transferAutoEx)
			        m_brctranscation.verifyTransferAutoEx(m_t.sender(), m_batch_params._operation, (m_baseGasRequired + transationTool::c_add_value[transationTool::op_type::transferAutoEx]) * m_t.gasPrice(), m_t.sha3(), m_envInfo);
                else if(m_batch_params._type == transationTool::op_type::modifyMinerGasPrice)
                    m_brctranscation.verifyModifyMinerGasPrice(m_t.sender(), m_envInfo.number(), m_batch_params._operation);
            }
			catch(VerifyVoteField &ex){
                cdebug << "verifyVote field ! ";
                cdebug << " except:" << ex.what();
				m_excepted = TransactionException::VerifyVoteField;
				BOOST_THROW_EXCEPTION(VerifyVoteField() << errinfo_comment(*boost::get_error_info<errinfo_comment>(ex)));
			}
			catch(VerifyPendingOrderFiled const& _v){
				BOOST_THROW_EXCEPTION(VerifyPendingOrderFiled() << errinfo_comment(*boost::get_error_info<errinfo_comment>(_v)));
			}
			catch(CancelPendingOrderFiled const& _c){
				BOOST_THROW_EXCEPTION(CancelPendingOrderFiled() << errinfo_comment(*boost::get_error_info<errinfo_comment>(_c)));
			}
			catch(receivingincomeFiled const& _r)
            {
			    BOOST_THROW_EXCEPTION(receivingincomeFiled() << errinfo_comment(*boost::get_error_info<errinfo_comment>(_r)));
            }
            catch(ChangeMinerFailed const& _r)
            {
                BOOST_THROW_EXCEPTION(ChangeMinerFailed() << errinfo_comment(*boost::get_error_info<errinfo_comment>(_r)));
            }
		}
	}
}

bool Executive::execute()
{
	m_needRefundGas = m_totalGas - (u256)m_baseGasRequired * m_t.gasPrice() - m_addCostValue ;
    assert(m_t.gas() >= (u256)m_baseGasRequired);
    if (m_t.isCreation())
        return create(m_t.sender(), m_t.value(), m_t.gasPrice(),
            m_t.gas() - (u256)m_baseGasRequired, &m_t.data(), m_t.sender());
    else
    {
        return call(m_t.receiveAddress(), m_t.sender(), m_t.value(), m_t.gasPrice(),
            bytesConstRef(&m_t.data()), m_t.gas() - (u256)m_baseGasRequired);

    }
}

bool Executive::call(Address const& _receiveAddress, Address const& _senderAddress,
    u256 const& _value, u256 const& _gasPrice, bytesConstRef _data, u256 const& _gas)
{
    CallParameters params{
        _senderAddress, _receiveAddress, _receiveAddress, _value, _value, _gas, _data, {}};
    return call(params, _gasPrice, _senderAddress);
}

bool Executive::call(CallParameters const& _p, u256 const& _gasPrice, Address const& _origin)
{
    // If external transaction.
    if (m_t)
    {
        // FIXME: changelog contains unrevertable balance change that paid
        //        for the transaction.
        // Increment associated nonce for sender.
        if (_p.senderAddress != MaxAddress || m_envInfo.number() < m_sealEngine.chainParams().experimentalForkBlock)  {// EIP86{
            m_s.incNonce(_p.senderAddress);
        }

    }

    m_savepoint = m_s.savepoint();

    if (m_sealEngine.isPrecompiled(_p.codeAddress, m_envInfo.number()))
    {
        bigint g = m_sealEngine.costOfPrecompiled(_p.codeAddress, _p.data, m_envInfo.number());
        if (_p.gas < g)
        {
            m_excepted = TransactionException::OutOfGasBase;
            if (m_envInfo.number() >= m_sealEngine.chainParams().EIP158ForkBlock)
                m_s.addBalance(_p.codeAddress, 0);

            return true;  // true actually means "all finished - nothing more to be done regarding
                          // go().
        }
        else
        {
            m_gas = (u256)(_p.gas - g);
            bytes output;
            bool success;
            tie(success, output) =
                m_sealEngine.executePrecompiled(_p.codeAddress, _p.data, m_envInfo.number());
            size_t outputSize = output.size();
            m_output = owning_bytes_ref{std::move(output), 0, outputSize};
            if (!success)
            {
                m_gas = 0;
                m_excepted = TransactionException::OutOfGas;
                return true;  // true means no need to run go().
            }
        }
    }
    else
    {
        m_gas = _p.gas;
        if (m_s.addressHasCode(_p.codeAddress))
        {
            bytes const& c = m_s.code(_p.codeAddress);
            h256 codeHash = m_s.codeHash(_p.codeAddress);
            m_ext = make_shared<ExtVM>(m_s, m_envInfo, m_sealEngine, _p.receiveAddress,
                _p.senderAddress, _origin, _p.apparentValue, _gasPrice, _p.data, &c, codeHash,
                m_depth, false, _p.staticCall);
        }
    }
    // Transfer brcer.
    if (!m_t.isVoteTranction())
    {
       m_s.transferBRC(_p.senderAddress, _p.receiveAddress, _p.valueTransfer);
    }
    else
    {
		if(m_batch_params.size() < 1)
			return false;
		transationTool::op_type  _type = m_batch_params._type;

        switch (_type){
            case transationTool::op_type::vote:{
                m_s.execute_vote(m_t.sender(), m_batch_params._operation, m_envInfo);
                break;
            }
            case transationTool::op_type::brcTranscation:{
                m_s.execute_transfer_BRCs(m_t.sender(), m_batch_params._operation);
                break;
            }
            case transationTool::op_type::pendingOrder:{
                m_s.pendingOrders(m_t.sender(), m_envInfo.timestamp(), m_t.sha3(), m_batch_params._operation);

                break;
            }
            case transationTool::op_type::cancelPendingOrder:{
                m_s.cancelPendingOrders(m_batch_params._operation);
                break;
            }
            case transationTool::op_type::changeMiner:{
                m_s.changeMiner(m_batch_params._operation);
                if(m_envInfo.number() > config::gasPriceHeight())
                {
                    m_s.changeMinerModifyGasPrice(m_batch_params._operation);
                }
                break;
            }
            case transationTool::op_type::receivingincome:{
                m_s.receivingIncome(m_t.sender(), m_batch_params._operation ,m_envInfo.number());
                break;
            }
            case transationTool::op_type::transferAutoEx:
            {
                m_s.transferAutoEx(m_batch_params._operation, m_t.sha3(), m_envInfo.timestamp(), (m_baseGasRequired + transationTool::c_add_value[transationTool::op_type::transferAutoEx]) * m_t.gasPrice());
                break;
            }
            case transationTool::op_type::modifyMinerGasPrice:
            {
                m_s.modifyGasPrice(m_batch_params._operation);
            }
            break;
            default:
                //TODO: unkown null.
                assert(1);
        }

		m_batch_params.clear();
		return true;
    }
    return !m_ext;
}

bool Executive::create(Address const& _txSender, u256 const& _endowment, u256 const& _gasPrice,
    u256 const& _gas, bytesConstRef _init, Address const& _origin)
{
    // Contract creation by an external account is the same as CREATE opcode
    return createOpcode(_txSender, _endowment, _gasPrice, _gas, _init, _origin);
}

bool Executive::createOpcode(Address const& _sender, u256 const& _endowment, u256 const& _gasPrice,
    u256 const& _gas, bytesConstRef _init, Address const& _origin)
{
    u256 nonce = m_s.getNonce(_sender);
    m_newAddress = right160(sha3(rlpList(_sender, nonce)));
    return executeCreate(_sender, _endowment, _gasPrice, _gas, _init, _origin);
}

bool Executive::create2Opcode(Address const& _sender, u256 const& _endowment, u256 const& _gasPrice,
    u256 const& _gas, bytesConstRef _init, Address const& _origin, u256 const& _salt)
{
    m_newAddress =
        right160(sha3(bytes{0xff} + _sender.asBytes() + toBigEndian(_salt) + sha3(_init)));
    return executeCreate(_sender, _endowment, _gasPrice, _gas, _init, _origin);
}

bool Executive::executeCreate(Address const& _sender, u256 const& _endowment, u256 const& _gasPrice,
    u256 const& _gas, bytesConstRef _init, Address const& _origin)
{
    if (_sender != MaxAddress ||
        m_envInfo.number() < m_sealEngine.chainParams().experimentalForkBlock)  // EIP86
        m_s.incNonce(_sender);

    m_savepoint = m_s.savepoint();

    m_isCreation = true;

    // We can allow for the reverted state (i.e. that with which m_ext is constructed) to contain
    // the m_orig.address, since we delete it explicitly if we decide we need to revert.

    m_gas = _gas;
    bool accountAlreadyExist = (m_s.addressHasCode(m_newAddress) || m_s.getNonce(m_newAddress) > 0);
    if (accountAlreadyExist)
    {
        LOG(m_detailsLogger) << "Address already used: " << m_newAddress;
        m_gas = 0;
        m_excepted = TransactionException::AddressAlreadyUsed;
        revert();
        m_ext = {};  // cancel the _init execution if there are any scheduled.
        return !m_ext;
    }

    // Transfer brcer before deploying the code. This will also create new
    // account if it does not exist yet.
    m_s.transferBRC(_sender, m_newAddress, _endowment);

    u256 newNonce = m_s.requireAccountStartNonce();
    if (m_envInfo.number() >= m_sealEngine.chainParams().EIP158ForkBlock)
        newNonce += 1;
    m_s.setNonce(m_newAddress, newNonce);

    m_s.clearStorage(m_newAddress);

    // Schedule _init execution if not empty.
    if (!_init.empty())
        m_ext = make_shared<ExtVM>(m_s, m_envInfo, m_sealEngine, m_newAddress, _sender, _origin,
            _endowment, _gasPrice, bytesConstRef(), _init, sha3(_init), m_depth, true, false);

    return !m_ext;
}

OnOpFunc Executive::simpleTrace()
{
    Logger& traceLogger = m_vmTraceLogger;

    return [&traceLogger](uint64_t steps, uint64_t PC, Instruction inst, bigint newMemSize,
               bigint gasCost, bigint gas, VMFace const* _vm, ExtVMFace const* voidExt) {
        ExtVM const& ext = *static_cast<ExtVM const*>(voidExt);
        auto vm = dynamic_cast<LegacyVM const*>(_vm);

        ostringstream o;
        if (vm)
            LOG(traceLogger) << dumpStackAndMemory(*vm);
        LOG(traceLogger) << dumpStorage(ext);
        LOG(traceLogger) << " < " << dec << ext.depth << " : " << ext.myAddress << " : #" << steps
                         << " : " << hex << setw(4) << setfill('0') << PC << " : "
                         << instructionInfo(inst).name << " : " << dec << gas << " : -" << dec
                         << gasCost << " : " << newMemSize << "x32"
                         << " >";
    };
}

bool Executive::go(OnOpFunc const& _onOp)
{
    bool success = false;
    if (m_ext)
    {
#if BRC_TIMED_EXECUTIONS
        Timer t;
#endif
        try
        {
            // Create VM instance. Force Interpreter if tracing requested.
            auto vm = VMFactory::create();
            if (m_isCreation)
            {
                auto out = vm->exec(m_gas, *m_ext, _onOp);
                if (m_res)
                {
                    m_res->gasForDeposit = m_gas;
                    m_res->depositSize = out.size();
                }
                if (out.size() > m_ext->brcSchedule().maxCodeSize)
                    BOOST_THROW_EXCEPTION(OutOfGas());
                else if (out.size() * m_ext->brcSchedule().createDataGas <= m_gas)
                {
                    if (m_res)
                        m_res->codeDeposit = CodeDeposit::Success;
                    m_gas -= out.size() * m_ext->brcSchedule().createDataGas;
                }
                else
                {
                    if (m_ext->brcSchedule().exceptionalFailedCodeDeposit)
                        BOOST_THROW_EXCEPTION(OutOfGas());
                    else
                    {
                        if (m_res)
                            m_res->codeDeposit = CodeDeposit::Failed;
                        out = {};
                    }
                }
                if (m_res)
                    m_res->output = out.toVector();  // copy output to execution result
                m_s.setCode(m_ext->myAddress, out.toVector());
            }
            else{
                m_output = vm->exec(m_gas, *m_ext, _onOp);

            }
            success = true;
        }
        catch (RevertInstruction& _e)
        {
            LOG(m_detailsLogger) << "RevertInstruction. " << _e.what() << " output " << _e.output().toString();
            revert();
            m_output = _e.output();
            m_excepted = TransactionException::RevertInstruction;
        }
        catch (VMException const& _e)
        {
            LOG(m_detailsLogger) << "Safe VM Exception. " << diagnostic_information(_e);
            m_gas = 0;
            m_excepted = toTransactionException(_e);
            revert();
        }
        catch (InternalVMError const& _e)
        {
            cerror << "Internal VM Error (BVMC status code: "
                   << *boost::get_error_info<errinfo_bvmcStatusCode>(_e) << ")";
            revert();
            throw;
        }
        catch (Exception const& _e)
        {
            // TODO: AUDIT: check that this can never reasonably happen. Consider what to do if it
            // does.
            cerror << "Unexpected exception in VM. There may be a bug in this implementation. "
                   << diagnostic_information(_e);
            exit(1);
            // Another solution would be to reject this transaction, but that also
            // has drawbacks. Essentially, the amount of ram has to be increased here.
        }
        catch (std::exception const& _e)
        {
            // TODO: AUDIT: check that this can never reasonably happen. Consider what to do if it
            // does.
            cerror << "Unexpected std::exception in VM. Not enough RAM? " << _e.what();
            exit(1);
            // Another solution would be to reject this transaction, but that also
            // has drawbacks. Essentially, the amount of ram has to be increased here.
        }

        if (m_res && m_output)
            // Copy full output:
            m_res->output = m_output.toVector();

#if BRC_TIMED_EXECUTIONS
        cnote << "VM took:" << t.elapsed() << "; gas used: " << (sgas - m_endGas);
#endif
    }
    return success;
}

bool Executive::finalize()
{
    if (m_ext)
    {
        // Accumulate refunds for suicides.
        m_ext->sub.refunds += m_ext->brcSchedule().suicideRefundGas * m_ext->sub.suicides.size();

        // Refunds must be applied before the miner gets the fees.
        assert(m_ext->sub.refunds >= 0);
        int64_t maxRefund = (static_cast<int64_t>(m_t.gas()) - static_cast<int64_t>(m_gas)) / 2;
        m_gas += min(maxRefund, m_ext->sub.refunds);
    }

    if (m_t)
    {
        m_s.subBalance(m_t.sender(), m_totalGas - m_needRefundGas);
        m_s.addBlockReward(m_envInfo.author(), m_envInfo.number(), m_totalGas - m_needRefundGas);

        // updata about author mapping_address
        // TODO fork code
        if (m_envInfo.number() >= config::newChangeHeight()) {
            auto miner_mapping = m_s.minerMapping(m_envInfo.author());
            Address up_addr = miner_mapping.first == Address() ? m_envInfo.author() : miner_mapping.first;
            m_s.try_new_vote_snapshot(up_addr, m_envInfo.number());
            m_s.addCooikeIncomeNum(up_addr, m_totalGas - m_needRefundGas);
        }
        else{
            m_s.try_new_vote_snapshot(m_envInfo.author(), m_envInfo.number());
            m_s.addCooikeIncomeNum(m_envInfo.author(), m_totalGas - m_needRefundGas);
        }
    }

    // Suicides...
    if (m_ext)
        for (auto a : m_ext->sub.suicides)
            m_s.kill(a);

    // Logs..
    if (m_ext)
        m_logs = m_ext->sub.logs;

    if (m_res)  // Collect results
    {
        m_res->gasUsed = gasUsed();
        m_res->excepted = m_excepted;  // TODO: m_except is used only in ExtVM::call
        m_res->newAddress = m_newAddress;
        m_res->gasRefunded = m_ext ? m_ext->sub.refunds : 0;
    }
    return (m_excepted == TransactionException::None);
}

void Executive::revert()
{
    if (m_ext)
        m_ext->sub.clear();

    // Set result address to the null one.
    m_newAddress = {};
    m_s.rollback(m_savepoint);
}

void dev::brc::Executive::setCallParameters(Address const& _senderAddress,
    Address const& _codeAddress, Address const& _receiveAddress, u256 _valueTransfer,
    u256 _apparentValue, u256 _gas, bytesConstRef const& _data, OnOpFunc _onOpFunc)
{
    m_callParameters.senderAddress = _senderAddress;
    m_callParameters.codeAddress = _codeAddress;
    m_callParameters.receiveAddress = _receiveAddress;
    m_callParameters.valueTransfer = _valueTransfer;
    m_callParameters.apparentValue = _apparentValue;
    m_callParameters.gas = _gas;
    m_callParameters.data = _data;
    m_callParameters.onOp = _onOpFunc;
}
