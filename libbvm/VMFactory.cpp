#include "VMFactory.h"
#include "BVMC.h"
#include "LegacyVM.h"

#include <libbrcd-interpreter/interpreter.h>

#include <bvmc/loader.h>

namespace po = boost::program_options;

namespace dev
{
namespace brc
{
namespace
{
auto g_kind = VMKind::Legacy;

/// The pointer to BVMC create function in DLL BVMC VM.
///
/// This variable is only written once when processing command line arguments,
/// so access is thread-safe.
std::unique_ptr<BVMC> g_bvmcDll;

/// A helper type to build the tabled of VM implementations.
///
/// More readable than std::tuple.
/// Fields are not initialized to allow usage of construction with initializer lists {}.
struct VMKindTableEntry
{
    VMKind kind;
    const char* name;
};

/// The table of available VM implementations.
///
/// We don't use a map to avoid complex dynamic initialization. This list will never be long,
/// so linear search only to parse command line arguments is not a problem.
VMKindTableEntry vmKindsTable[] = {
    {VMKind::Interpreter, "interpreter"},
    {VMKind::Legacy, "legacy"},
};

void setVMKind(const std::string& _name)
{
    for (auto& entry : vmKindsTable)
    {
        // Try to find a match in the table of VMs.
        if (_name == entry.name)
        {
            g_kind = entry.kind;
            return;
        }
    }

    // If no match for predefined VM names, try loading it as an BVMC VM DLL.
    g_kind = VMKind::DLL;

    // Release previous instance
    g_bvmcDll.reset();

    bvmc_loader_error_code ec;
    bvmc_instance *instance = bvmc_load_and_create(_name.c_str(), &ec);
    assert(ec == BVMC_LOADER_SUCCESS || instance == nullptr);

    switch (ec)
    {
    case BVMC_LOADER_SUCCESS:
        break;
    case BVMC_LOADER_CANNOT_OPEN:
        BOOST_THROW_EXCEPTION(
            po::validation_error(po::validation_error::invalid_option_value, "vm", _name, 1));
    case BVMC_LOADER_SYMBOL_NOT_FOUND:
        BOOST_THROW_EXCEPTION(std::system_error(std::make_error_code(std::errc::invalid_seek),
            "loading " + _name + " failed: BVMC create function not found"));
    case BVMC_LOADER_ABI_VERSION_MISMATCH:
        BOOST_THROW_EXCEPTION(std::system_error(std::make_error_code(std::errc::invalid_argument),
            "loading " + _name + " failed: BVMC ABI version mismatch"));
    default:
        BOOST_THROW_EXCEPTION(
            std::system_error(std::error_code(static_cast<int>(ec), std::generic_category()),
                "loading " + _name + " failed"));
    }

    g_bvmcDll.reset(new BVMC{instance});

    cnote << "Loaded BVMC module: " << g_bvmcDll->name() << " " << g_bvmcDll->version() << " ("
          << _name << ")";
}
}  // namespace

namespace
{
/// The name of the program option --bvmc. The boost will trim the tailing
/// space and we can reuse this variable in exception message.
const char c_bvmcPrefix[] = "bvmc ";

/// The list of BVMC options stored as pairs of (name, value).
std::vector<std::pair<std::string, std::string>> s_bvmcOptions;

/// The additional parser for BVMC options. The options should look like
/// `--bvmc name=value` or `--bvmc=name=value`. The boost pass the strings
/// of `name=value` here. This function splits the name and value or reports
/// the syntax error if the `=` character is missing.
void parseBvmcOptions(const std::vector<std::string>& _opts)
{
    for (auto& s : _opts)
    {
        auto separatorPos = s.find('=');
        if (separatorPos == s.npos)
            throw po::invalid_syntax{po::invalid_syntax::missing_parameter, c_bvmcPrefix + s};
        auto name = s.substr(0, separatorPos);
        auto value = s.substr(separatorPos + 1);
        s_bvmcOptions.emplace_back(std::move(name), std::move(value));
    }
}
}  // namespace

std::vector<std::pair<std::string, std::string>>& bvmcOptions() noexcept
{
    return s_bvmcOptions;
};

po::options_description vmProgramOptions(unsigned _lineLength)
{
    // It must be a static object because boost expects const char*.
    static const std::string description = [] {
        std::string names;
        for (auto& entry : vmKindsTable)
        {
            if (!names.empty())
                names += ", ";
            names += entry.name;
        }

        return "Select VM implementation. Available options are: " + names + ".";
    }();

    po::options_description opts("VM OPTIONS", _lineLength);
    auto add = opts.add_options();

    add("vm",
        po::value<std::string>()
            ->value_name("<name>|<path>")
            ->default_value("legacy")
            ->notifier(setVMKind),
        description.data());

    add(c_bvmcPrefix,
        po::value<std::vector<std::string>>()
            ->multitoken()
            ->value_name("<option>=<value>")
            ->notifier(parseBvmcOptions),
        "BVMC option\n");

    return opts;
}


VMPtr VMFactory::create()
{
    return create(g_kind);
}

VMPtr VMFactory::create(VMKind _kind)
{
    static const auto default_delete = [](VMFace * _vm) noexcept { delete _vm; };
    static const auto null_delete = [](VMFace*) noexcept {};

    switch (_kind)
    {
    case VMKind::Interpreter:
        return {new BVMC{bvmc_create_interpreter()}, default_delete};
    case VMKind::DLL:
        assert(g_bvmcDll != nullptr);
        // Return "fake" owning pointer to global BVMC DLL VM.
        return {g_bvmcDll.get(), null_delete};
    case VMKind::Legacy:
    default:
        return {new LegacyVM, default_delete};
    }
}
}  // namespace brc
}  // namespace dev
