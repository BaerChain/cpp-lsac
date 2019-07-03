#pragma once

#include <unordered_map>
#include <functional>
#include <libdevcore/CommonData.h>
#include <libdevcore/Exceptions.h>

namespace dev
{
namespace brc
{

using PrecompiledExecutor = std::function<std::pair<bool, bytes>(bytesConstRef _in)>;
using PrecompiledPricer = std::function<bigint(bytesConstRef _in)>;

DEV_SIMPLE_EXCEPTION(ExecutorNotFound);
DEV_SIMPLE_EXCEPTION(PricerNotFound);

class PrecompiledRegistrar
{
public:
	/// Get the executor object for @a _name function or @throw ExecutorNotFound if not found.
	static PrecompiledExecutor const& executor(std::string const& _name);

	/// Get the price calculator object for @a _name function or @throw PricerNotFound if not found.
	static PrecompiledPricer const& pricer(std::string const& _name);

	/// Register an executor. In general just use BRC_REGISTER_PRECOMPILED.
	static PrecompiledExecutor registerExecutor(std::string const& _name, PrecompiledExecutor const& _exec) { return (get()->m_execs[_name] = _exec); }
	/// Unregister an executor. Shouldn't generally be necessary.
	static void unregisterExecutor(std::string const& _name) { get()->m_execs.erase(_name); }

	/// Register a pricer. In general just use BRC_REGISTER_PRECOMPILED_PRICER.
	static PrecompiledPricer registerPricer(std::string const& _name, PrecompiledPricer const& _exec) { return (get()->m_pricers[_name] = _exec); }
	/// Unregister a pricer. Shouldn't generally be necessary.
	static void unregisterPricer(std::string const& _name) { get()->m_pricers.erase(_name); }

private:
	static PrecompiledRegistrar* get() { if (!s_this) s_this = new PrecompiledRegistrar; return s_this; }

	std::unordered_map<std::string, PrecompiledExecutor> m_execs;
	std::unordered_map<std::string, PrecompiledPricer> m_pricers;
	static PrecompiledRegistrar* s_this;
};

// TODO: unregister on unload with a static object.
#define BRC_REGISTER_PRECOMPILED(Name) static std::pair<bool, bytes> __brc_registerPrecompiledFunction ## Name(bytesConstRef _in); static PrecompiledExecutor __brc_registerPrecompiledFactory ## Name = ::dev::brc::PrecompiledRegistrar::registerExecutor(#Name, &__brc_registerPrecompiledFunction ## Name); static std::pair<bool, bytes> __brc_registerPrecompiledFunction ## Name
#define BRC_REGISTER_PRECOMPILED_PRICER(Name) static bigint __brc_registerPricerFunction ## Name(bytesConstRef _in); static PrecompiledPricer __brc_registerPricerFactory ## Name = ::dev::brc::PrecompiledRegistrar::registerPricer(#Name, &__brc_registerPricerFunction ## Name); static bigint __brc_registerPricerFunction ## Name
}
}
