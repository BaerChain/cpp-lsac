#pragma once

#include <libbrccore/Common.h>

namespace dev
{
namespace brc
{

class Block;
class BlockChain;

enum class TransactionPriority
{
	Lowest = 0,
	Low = 2,
	Medium = 4,
	High = 6,
	Highest = 8
};

static const u256 DefaultGasPrice = 20 * shannon;
static const u256 AverageGasPrice = 12 * wei;

class GasPricer
{
public:
	GasPricer() = default;
	virtual ~GasPricer() = default;

	virtual u256 ask(Block const&) const = 0;
	virtual u256 bid(TransactionPriority _p = TransactionPriority::Medium) const = 0;

	virtual void update(BlockChain const&) {}
};

class TrivialGasPricer: public GasPricer
{
public:
	TrivialGasPricer() = default;
	TrivialGasPricer(u256 const& _ask, u256 const& _bid): m_ask(_ask), m_bid(_bid) {}

	void setAsk(u256 const& _ask) { m_ask = _ask; }
	void setBid(u256 const& _bid) { m_bid = _bid; }

	u256 ask() const { return m_ask; }
	u256 ask(Block const&) const override { return m_ask; }
	u256 bid(TransactionPriority = TransactionPriority::Medium) const override { return m_bid; }

private:
	u256 m_ask = DefaultGasPrice;
	u256 m_bid = DefaultGasPrice;
};

}
}
