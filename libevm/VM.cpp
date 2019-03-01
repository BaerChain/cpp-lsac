/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file VM.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "VM.h"
#include <libethereum/ExtVM.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

struct InstructionMetric
{
	int gasPriceTier;
	int args;
	int ret;
};

static array<InstructionMetric, 256> metrics()
{
	array<InstructionMetric, 256> s_ret;
	for (unsigned i = 0; i < 256; ++i)
	{
		InstructionInfo inst = instructionInfo((Instruction)i);
		s_ret[i].gasPriceTier = inst.gasPriceTier;
		s_ret[i].args = inst.args;
		s_ret[i].ret = inst.ret;
	}
	return s_ret;
}

void VM::require(u256 _n, u256 _d)
{
	if (m_stack.size() < _n)
	{
		if (m_onFail)
			m_onFail();
		BOOST_THROW_EXCEPTION(StackUnderflow() << RequirementError((bigint)_n, (bigint)m_stack.size()));
	}
	if (m_stack.size() - _n + _d > m_schedule->stackLimit)
	{
		if (m_onFail)
			m_onFail();
		BOOST_THROW_EXCEPTION(OutOfStack() << RequirementError((bigint)(_d - _n), (bigint)m_stack.size()));
	}
}

void VM::checkRequirements(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp, Instruction _inst)
{
	static const auto c_metrics = metrics();
	auto& metric = c_metrics[static_cast<size_t>(_inst)];

	// Pre-homestead
	if (!m_schedule->haveDelegateCall && _inst == Instruction::DELEGATECALL)
		BOOST_THROW_EXCEPTION(BadInstruction());

	if (metric.gasPriceTier == InvalidTier)
		BOOST_THROW_EXCEPTION(BadInstruction());

	// FEES...
	bigint runGas = m_schedule->tierStepGas[metric.gasPriceTier];
	bigint newTempSize = m_temp.size();
	bigint copySize = 0;

	// should work, but just seems to result in immediate errorless exit on initial execution. yeah. weird.
	//m_onFail = std::function<void()>(onOperation);

	require(metric.args, metric.ret);

	auto onOperation = [&]()
	{
		if (_onOp)
			_onOp(m_steps, _inst, newTempSize > m_temp.size() ? (newTempSize - m_temp.size()) / 32 : bigint(0), runGas, io_gas, this, &_ext);
	};

	auto memNeed = [](u256 _offset, dev::u256 _size) { return _size ? (bigint)_offset + _size : (bigint)0; };
	
	switch (_inst)
	{
	case Instruction::SSTORE:
		if (!_ext.store(m_stack.back()) && m_stack[m_stack.size() - 2])
			runGas = m_schedule->sstoreSetGas;
		else if (_ext.store(m_stack.back()) && !m_stack[m_stack.size() - 2])
		{
			runGas = m_schedule->sstoreResetGas;
			_ext.sub.refunds += m_schedule->sstoreRefundGas;
		}
		else
			runGas = m_schedule->sstoreResetGas;
		break;

	case Instruction::SLOAD:
		runGas = m_schedule->sloadGas;
		break;

	// These all operate on memory and therefore potentially expand it:
	case Instruction::MSTORE:
		newTempSize = (bigint)m_stack.back() + 32;
		break;
	case Instruction::MSTORE8:
		newTempSize = (bigint)m_stack.back() + 1;
		break;
	case Instruction::MLOAD:
		newTempSize = (bigint)m_stack.back() + 32;
		break;
	case Instruction::RETURN:
		newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 2]);
		break;
	case Instruction::SHA3:
		runGas = m_schedule->sha3Gas + ((bigint)m_stack[m_stack.size() - 2] + 31) / 32 * m_schedule->sha3WordGas;
		newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 2]);
		break;
	case Instruction::CALLDATACOPY:
		copySize = m_stack[m_stack.size() - 3];
		newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 3]);
		break;
	case Instruction::CODECOPY:
		copySize = m_stack[m_stack.size() - 3];
		newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 3]);
		break;
	case Instruction::EXTCODECOPY:
		copySize = m_stack[m_stack.size() - 4];
		newTempSize = memNeed(m_stack[m_stack.size() - 2], m_stack[m_stack.size() - 4]);
		break;

	case Instruction::JUMPDEST:
		runGas = 1;
		break;

	case Instruction::LOG0:
	case Instruction::LOG1:
	case Instruction::LOG2:
	case Instruction::LOG3:
	case Instruction::LOG4:
	{
		unsigned n = (unsigned)_inst - (unsigned)Instruction::LOG0;
		runGas = m_schedule->logGas + m_schedule->logTopicGas * n + (bigint)m_schedule->logDataGas * m_stack[m_stack.size() - 2];
		newTempSize = memNeed(m_stack[m_stack.size() - 1], m_stack[m_stack.size() - 2]);
		break;
	}

	case Instruction::CALL:
	case Instruction::CALLCODE:
	case Instruction::DELEGATECALL:
	{
		runGas = (bigint)m_stack[m_stack.size() - 1] + m_schedule->callGas;

		if (_inst == Instruction::CALL && !_ext.exists(asAddress(m_stack[m_stack.size() - 2])))
			runGas += m_schedule->callNewAccountGas;

		if (_inst != Instruction::DELEGATECALL && m_stack[m_stack.size() - 3] > 0)
			runGas += m_schedule->callValueTransferGas;

		unsigned sizesOffset = _inst == Instruction::DELEGATECALL ? 3 : 4;
		newTempSize = std::max(
			memNeed(m_stack[m_stack.size() - sizesOffset - 2], m_stack[m_stack.size() - sizesOffset - 3]),
			memNeed(m_stack[m_stack.size() - sizesOffset], m_stack[m_stack.size() - sizesOffset - 1])
		);
		break;
	}
	case Instruction::CREATE:
	{
		newTempSize = memNeed(m_stack[m_stack.size() - 2], m_stack[m_stack.size() - 3]);
		runGas = m_schedule->createGas;
		break;
	}
	case Instruction::EXP:
	{
		auto expon = m_stack[m_stack.size() - 2];
		runGas = m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8));
		break;
	}
	default:;
	}

	auto gasForMem = [=](bigint _size) -> bigint
	{
		bigint s = _size / 32;
		return (bigint)m_schedule->memoryGas * s + s * s / m_schedule->quadCoeffDiv;
	};

	newTempSize = (newTempSize + 31) / 32 * 32;
	if (newTempSize > m_temp.size())
		runGas += gasForMem(newTempSize) - gasForMem(m_temp.size());
	runGas += m_schedule->copyGas * ((copySize + 31) / 32);

	onOperation();

	if (io_gas < runGas)
		BOOST_THROW_EXCEPTION(OutOfGas());

	io_gas -= (u256)runGas;

	if (newTempSize > m_temp.size())
		m_temp.resize(newTempSize.convert_to<size_t>());
}

uint64_t VM::verifyJumpDest(u256 const& _dest, vector<uint64_t> const& _validDests)
{
	auto nextPC = _dest.convert_to<uint64_t>();
	if (!std::binary_search(_validDests.begin(), _validDests.end(), nextPC) || _dest > std::numeric_limits<uint64_t>::max())
		BOOST_THROW_EXCEPTION(BadJumpDestination());
	return nextPC;
}

void VM::copyDataToMemory(bytesConstRef _data)
{
	auto offset = m_stack.back().convert_to<size_t>();
	m_stack.pop_back();
	bigint bigIndex = m_stack.back();
	auto index = bigIndex.convert_to<size_t>();
	m_stack.pop_back();
	auto size = (m_stack.back()).convert_to<size_t>();
	m_stack.pop_back();

	size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;

	if (sizeToBeCopied > 0)
		std::memcpy(m_temp.data() + offset, _data.data() + index, sizeToBeCopied);
	if (size > sizeToBeCopied)
		std::memset(m_temp.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
}

template <class S> S divWorkaround(S const& _a, S const& _b)
{
	return (S)(bigint(_a) / bigint(_b));
}

template <class S> S modWorkaround(S const& _a, S const& _b)
{
	return (S)(bigint(_a) % bigint(_b));
}

bytesConstRef VM::execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	m_schedule = &_ext.evmSchedule();
	m_stack.reserve((m_schedule->stackLimit).convert_to<size_t>());

	for (size_t i = 0; i < _ext.code.size(); ++i)
	{
		if (_ext.code[i] == (byte)Instruction::JUMPDEST)
			m_jumpDests.push_back(i);
		else if (_ext.code[i] >= (byte)Instruction::PUSH1 && _ext.code[i] <= (byte)Instruction::PUSH32)
			i += _ext.code[i] - (size_t)Instruction::PUSH1 + 1;
	}

	m_steps = 0;
	for (auto nextPC = m_curPC + 1; true; m_curPC = nextPC, nextPC = m_curPC + 1, ++m_steps)
	{
		Instruction inst = (Instruction)_ext.getCode(m_curPC);
		checkRequirements(io_gas, _ext, _onOp, inst);

		switch (inst)
		{
		case Instruction::CREATE:
		{
			auto const& endowment = m_stack.back();
			m_stack.pop_back();
			unsigned initOff = m_stack.back().convert_to<unsigned>();
			m_stack.pop_back();
			unsigned initSize = m_stack.back().convert_to<unsigned>();
			m_stack.pop_back();

			if (_ext.balance(_ext.myAddress) >= endowment && _ext.depth < 1024)
				m_stack.push_back((u160)_ext.create(endowment, io_gas, bytesConstRef(m_temp.data() + initOff, initSize), _onOp));
			else
				m_stack.push_back(0);
			break;
		}
		case Instruction::CALL:
		case Instruction::CALLCODE:
		case Instruction::DELEGATECALL:
		{
			unique_ptr<CallParameters> callParams(new CallParameters());

			callParams->gas = m_stack.back();
			if (inst != Instruction::DELEGATECALL && m_stack[m_stack.size() - 3] > 0)
				callParams->gas += m_schedule->callStipend;
			m_stack.pop_back();

			callParams->codeAddress = asAddress(m_stack.back());
			m_stack.pop_back();

			if (inst == Instruction::DELEGATECALL)
			{
				callParams->apparentValue = _ext.value;
				callParams->valueTransfer = 0;
			}
			else
			{
				callParams->apparentValue = callParams->valueTransfer = m_stack.back();
				m_stack.pop_back();
			}

			unsigned inOff = m_stack.back().convert_to<unsigned>();
			m_stack.pop_back();
			unsigned inSize = m_stack.back().convert_to<unsigned>();
			m_stack.pop_back();
			unsigned outOff = m_stack.back().convert_to<unsigned>();
			m_stack.pop_back();
			unsigned outSize = m_stack.back().convert_to<unsigned>();
			m_stack.pop_back();

			if (_ext.balance(_ext.myAddress) >= callParams->valueTransfer && _ext.depth < 1024)
			{
				callParams->onOp = _onOp;
				callParams->senderAddress = inst == Instruction::DELEGATECALL ? _ext.caller : _ext.myAddress;
				callParams->receiveAddress = inst == Instruction::CALL ? callParams->codeAddress : _ext.myAddress;
				callParams->data = bytesConstRef(m_temp.data() + inOff, inSize);
				callParams->out = bytesRef(m_temp.data() + outOff, outSize);
				m_stack.push_back(_ext.call(*callParams));
			}
			else
				m_stack.push_back(0);

			io_gas += callParams->gas;
			break;
		}
		case Instruction::RETURN:
		{
			unsigned b = m_stack.back().convert_to<unsigned>();
			m_stack.pop_back();
			unsigned s = m_stack.back().convert_to<unsigned>();
			m_stack.pop_back();
			return bytesConstRef(m_temp.data() + b, s);
		}
		case Instruction::SUICIDE:
		{
			Address dest = asAddress(m_stack.back());
			_ext.suicide(dest);
			return bytesConstRef();
		}
		case Instruction::STOP:
			return bytesConstRef();
		default:
			nextPC = execOrdinaryOpcode(inst, io_gas, _ext);
		}
	}

	return bytesConstRef();
}

uint64_t VM::execOrdinaryOpcode(Instruction _inst, u256 &io_gas, ExtVMFace& _ext)
{
	uint64_t nextPC = m_curPC + 1;
	switch (_inst)
	{
	case Instruction::ADD:
		//pops two items and pushes S[-1] + S[-2] mod 2^256.
		m_stack[m_stack.size() - 2] += m_stack.back();
		m_stack.pop_back();
		break;
	case Instruction::MUL:
		//pops two items and pushes S[-1] * S[-2] mod 2^256.
		m_stack[m_stack.size() - 2] *= m_stack.back();
		m_stack.pop_back();
		break;
	case Instruction::SUB:
		m_stack[m_stack.size() - 2] = m_stack.back() - m_stack[m_stack.size() - 2];
		m_stack.pop_back();
		break;
	case Instruction::DIV:
		m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? divWorkaround(m_stack.back(), m_stack[m_stack.size() - 2]) : 0;
		m_stack.pop_back();
		break;
	case Instruction::SDIV:
		m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? s2u(divWorkaround(u2s(m_stack.back()), u2s(m_stack[m_stack.size() - 2]))) : 0;
		m_stack.pop_back();
		break;
	case Instruction::MOD:
		m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? modWorkaround(m_stack.back(), m_stack[m_stack.size() - 2]) : 0;
		m_stack.pop_back();
		break;
	case Instruction::SMOD:
		m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? s2u(modWorkaround(u2s(m_stack.back()), u2s(m_stack[m_stack.size() - 2]))) : 0;
		m_stack.pop_back();
		break;
	case Instruction::EXP:
	{
		auto base = m_stack.back();
		auto expon = m_stack[m_stack.size() - 2];
		m_stack.pop_back();
		m_stack.back() = (u256)boost::multiprecision::powm((bigint)base, (bigint)expon, bigint(1) << 256);
		break;
	}
	case Instruction::NOT:
		m_stack.back() = ~m_stack.back();
		break;
	case Instruction::LT:
		m_stack[m_stack.size() - 2] = m_stack.back() < m_stack[m_stack.size() - 2] ? 1 : 0;
		m_stack.pop_back();
		break;
	case Instruction::GT:
		m_stack[m_stack.size() - 2] = m_stack.back() > m_stack[m_stack.size() - 2] ? 1 : 0;
		m_stack.pop_back();
		break;
	case Instruction::SLT:
		m_stack[m_stack.size() - 2] = u2s(m_stack.back()) < u2s(m_stack[m_stack.size() - 2]) ? 1 : 0;
		m_stack.pop_back();
		break;
	case Instruction::SGT:
		m_stack[m_stack.size() - 2] = u2s(m_stack.back()) > u2s(m_stack[m_stack.size() - 2]) ? 1 : 0;
		m_stack.pop_back();
		break;
	case Instruction::EQ:
		m_stack[m_stack.size() - 2] = m_stack.back() == m_stack[m_stack.size() - 2] ? 1 : 0;
		m_stack.pop_back();
		break;
	case Instruction::ISZERO:
		m_stack.back() = m_stack.back() ? 0 : 1;
		break;
	case Instruction::AND:
		m_stack[m_stack.size() - 2] = m_stack.back() & m_stack[m_stack.size() - 2];
		m_stack.pop_back();
		break;
	case Instruction::OR:
		m_stack[m_stack.size() - 2] = m_stack.back() | m_stack[m_stack.size() - 2];
		m_stack.pop_back();
		break;
	case Instruction::XOR:
		m_stack[m_stack.size() - 2] = m_stack.back() ^ m_stack[m_stack.size() - 2];
		m_stack.pop_back();
		break;
	case Instruction::BYTE:
		m_stack[m_stack.size() - 2] = m_stack.back() < 32 ? (m_stack[m_stack.size() - 2] >> (8 * (31 - m_stack.back())).convert_to<unsigned>()) & 0xff : 0;
		m_stack.pop_back();
		break;
	case Instruction::ADDMOD:
		m_stack[m_stack.size() - 3] = m_stack[m_stack.size() - 3] ? u256((bigint(m_stack.back()) + bigint(m_stack[m_stack.size() - 2])) % m_stack[m_stack.size() - 3]) : 0;
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::MULMOD:
		m_stack[m_stack.size() - 3] = m_stack[m_stack.size() - 3] ? u256((bigint(m_stack.back()) * bigint(m_stack[m_stack.size() - 2])) % m_stack[m_stack.size() - 3]) : 0;
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::SIGNEXTEND:
		if (m_stack.back() < 31)
		{
			auto testBit = m_stack.back().convert_to<unsigned>() * 8 + 7;
			u256& number = m_stack[m_stack.size() - 2];
			u256 mask = ((u256(1) << testBit) - 1);
			if (boost::multiprecision::bit_test(number, testBit))
				number |= ~mask;
			else
				number &= mask;
		}
		m_stack.pop_back();
		break;
	case Instruction::SHA3:
	{
		unsigned inOff = m_stack.back().convert_to<unsigned>();
		m_stack.pop_back();
		unsigned inSize = m_stack.back().convert_to<unsigned>();
		m_stack.pop_back();
		m_stack.push_back(sha3(bytesConstRef(m_temp.data() + inOff, inSize)));
		break;
	}
	case Instruction::ADDRESS:
		m_stack.push_back(fromAddress(_ext.myAddress));
		break;
	case Instruction::ORIGIN:
		m_stack.push_back(fromAddress(_ext.origin));
		break;
	case Instruction::BALANCE:
	{
		m_stack.back() = _ext.balance(asAddress(m_stack.back()));
		break;
	}
	case Instruction::CALLER:
		m_stack.push_back(fromAddress(_ext.caller));
		break;
	case Instruction::CALLVALUE:
		m_stack.push_back(_ext.value);
		break;
	case Instruction::CALLDATALOAD:
	{
		if ((bigint)m_stack.back() + 31 < _ext.data.size())
			m_stack.back() = (u256)*(h256 const*)(_ext.data.data() + m_stack.back().convert_to<size_t>());
		else if ((bigint)m_stack.back() >= _ext.data.size())
			m_stack.back() = u256();
		else
		{
			h256 r;
			for (uint64_t i = m_stack.back().convert_to<unsigned>(), e = m_stack.back().convert_to<unsigned>() + (uint64_t)32, j = 0; i < e; ++i, ++j)
				r[j] = i < _ext.data.size() ? _ext.data[i] : 0;
			m_stack.back() = (u256)r;
		}
		break;
	}
	case Instruction::CALLDATASIZE:
		m_stack.push_back(_ext.data.size());
		break;
	case Instruction::CODESIZE:
		m_stack.push_back(_ext.code.size());
		break;
	case Instruction::EXTCODESIZE:
		m_stack.back() = _ext.codeAt(asAddress(m_stack.back())).size();
		break;
	case Instruction::CALLDATACOPY:
		copyDataToMemory(_ext.data);
		break;
	case Instruction::CODECOPY:
		copyDataToMemory(&_ext.code);
		break;
	case Instruction::EXTCODECOPY:
	{
		auto a = asAddress(m_stack.back());
		m_stack.pop_back();
		copyDataToMemory(&_ext.codeAt(a));
		break;
	}
	case Instruction::GASPRICE:
		m_stack.push_back(_ext.gasPrice);
		break;
	case Instruction::BLOCKHASH:
		m_stack.back() = (u256)_ext.blockHash(m_stack.back());
		break;
	case Instruction::COINBASE:
		m_stack.push_back((u160)_ext.envInfo().author());
		break;
	case Instruction::TIMESTAMP:
		m_stack.push_back(_ext.envInfo().timestamp());
		break;
	case Instruction::NUMBER:
		m_stack.push_back(_ext.envInfo().number());
		break;
	case Instruction::DIFFICULTY:
		m_stack.push_back(_ext.envInfo().difficulty());
		break;
	case Instruction::GASLIMIT:
		m_stack.push_back(_ext.envInfo().gasLimit());
		break;
	case Instruction::POP:
		m_stack.pop_back();
		break;
	case Instruction::PUSH1:
	case Instruction::PUSH2:
	case Instruction::PUSH3:
	case Instruction::PUSH4:
	case Instruction::PUSH5:
	case Instruction::PUSH6:
	case Instruction::PUSH7:
	case Instruction::PUSH8:
	case Instruction::PUSH9:
	case Instruction::PUSH10:
	case Instruction::PUSH11:
	case Instruction::PUSH12:
	case Instruction::PUSH13:
	case Instruction::PUSH14:
	case Instruction::PUSH15:
	case Instruction::PUSH16:
	case Instruction::PUSH17:
	case Instruction::PUSH18:
	case Instruction::PUSH19:
	case Instruction::PUSH20:
	case Instruction::PUSH21:
	case Instruction::PUSH22:
	case Instruction::PUSH23:
	case Instruction::PUSH24:
	case Instruction::PUSH25:
	case Instruction::PUSH26:
	case Instruction::PUSH27:
	case Instruction::PUSH28:
	case Instruction::PUSH29:
	case Instruction::PUSH30:
	case Instruction::PUSH31:
	case Instruction::PUSH32:
	{
		int i = (int)_inst - (int)Instruction::PUSH1 + 1;
		nextPC = m_curPC + 1;
		m_stack.push_back(0);
		for (; i--; nextPC++)
			m_stack.back() = (m_stack.back() << 8) | _ext.getCode(nextPC);
		break;
	}
	case Instruction::JUMP:
		nextPC = verifyJumpDest(m_stack.back(), m_jumpDests);
		m_stack.pop_back();
		break;
	case Instruction::JUMPI:
		if (m_stack[m_stack.size() - 2])
			nextPC = verifyJumpDest(m_stack.back(), m_jumpDests);
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::DUP1:
	case Instruction::DUP2:
	case Instruction::DUP3:
	case Instruction::DUP4:
	case Instruction::DUP5:
	case Instruction::DUP6:
	case Instruction::DUP7:
	case Instruction::DUP8:
	case Instruction::DUP9:
	case Instruction::DUP10:
	case Instruction::DUP11:
	case Instruction::DUP12:
	case Instruction::DUP13:
	case Instruction::DUP14:
	case Instruction::DUP15:
	case Instruction::DUP16:
	{
		auto n = 1 + (unsigned)_inst - (unsigned)Instruction::DUP1;
		m_stack.push_back(m_stack[m_stack.size() - n]);
		break;
	}
	case Instruction::SWAP1:
	case Instruction::SWAP2:
	case Instruction::SWAP3:
	case Instruction::SWAP4:
	case Instruction::SWAP5:
	case Instruction::SWAP6:
	case Instruction::SWAP7:
	case Instruction::SWAP8:
	case Instruction::SWAP9:
	case Instruction::SWAP10:
	case Instruction::SWAP11:
	case Instruction::SWAP12:
	case Instruction::SWAP13:
	case Instruction::SWAP14:
	case Instruction::SWAP15:
	case Instruction::SWAP16:
	{
		auto n = (unsigned)_inst - (unsigned)Instruction::SWAP1 + 2;
		auto d = m_stack.back();
		m_stack.back() = m_stack[m_stack.size() - n];
		m_stack[m_stack.size() - n] = d;
		break;
	}
	case Instruction::MLOAD:
	{
		m_stack.back() = (u256)*(h256 const*)(m_temp.data() + m_stack.back().convert_to<unsigned>());
		break;
	}
	case Instruction::MSTORE:
	{
		*(h256*)&m_temp[m_stack.back().convert_to<unsigned>()] = (h256)m_stack[m_stack.size() - 2];
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	}
	case Instruction::MSTORE8:
	{
		m_temp[m_stack.back().convert_to<unsigned>()] = (m_stack[m_stack.size() - 2] & 0xff).convert_to<byte>();
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	}
	case Instruction::SLOAD:
		m_stack.back() = _ext.store(m_stack.back());
		break;
	case Instruction::SSTORE:
		_ext.setStore(m_stack.back(), m_stack[m_stack.size() - 2]);
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::PC:
		m_stack.push_back(m_curPC);
		break;
	case Instruction::MSIZE:
		m_stack.push_back(m_temp.size());
		break;
	case Instruction::GAS:
		m_stack.push_back(io_gas);
		break;
	case Instruction::JUMPDEST:
		break;
	case Instruction::LOG0:
		_ext.log({}, bytesConstRef(m_temp.data() + m_stack[m_stack.size() - 1].convert_to<unsigned>(), m_stack[m_stack.size() - 2].convert_to<unsigned>()));
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::LOG1:
		_ext.log({m_stack[m_stack.size() - 3]}, bytesConstRef(m_temp.data() + m_stack[m_stack.size() - 1].convert_to<unsigned>(), m_stack[m_stack.size() - 2].convert_to<unsigned>()));
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::LOG2:
		_ext.log({m_stack[m_stack.size() - 3], m_stack[m_stack.size() - 4]}, bytesConstRef(m_temp.data() + m_stack[m_stack.size() - 1].convert_to<unsigned>(), m_stack[m_stack.size() - 2].convert_to<unsigned>()));
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::LOG3:
		_ext.log({m_stack[m_stack.size() - 3], m_stack[m_stack.size() - 4], m_stack[m_stack.size() - 5]}, bytesConstRef(m_temp.data() + m_stack[m_stack.size() - 1].convert_to<unsigned>(), m_stack[m_stack.size() - 2].convert_to<unsigned>()));
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::LOG4:
		_ext.log({m_stack[m_stack.size() - 3], m_stack[m_stack.size() - 4], m_stack[m_stack.size() - 5], m_stack[m_stack.size() - 6]}, bytesConstRef(m_temp.data() + m_stack[m_stack.size() - 1].convert_to<unsigned>(), m_stack[m_stack.size() - 2].convert_to<unsigned>()));
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		m_stack.pop_back();
		break;
	case Instruction::CREATE:
	case Instruction::CALL:
	case Instruction::CALLCODE:
	case Instruction::DELEGATECALL:
	case Instruction::RETURN:
	case Instruction::SUICIDE:
	case Instruction::STOP:
		break; // These are handled above
	}
	return nextPC;
}
