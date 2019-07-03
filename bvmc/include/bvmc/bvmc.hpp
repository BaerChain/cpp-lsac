/// @file
/// BVMC C++ API - wrappers and bindings for C++.

#include <bvmc/bvmc.h>
#include <bvmc/helpers.h>

#include <initializer_list>
#include <utility>

namespace bvmc
{
class result : public bvmc_result
{
public:
    explicit result(bvmc_result const& res) noexcept : bvmc_result{res} {}

    ~result() noexcept
    {
        if (release)
            release(this);
    }

    result(result&& other) noexcept : bvmc_result{other}
    {
        // Disable releaser of the rvalue object.
        other.release = nullptr;
    }

    result(result const&) = delete;

    result& operator=(result other) noexcept
    {
        std::swap(*this, other);
        return *this;
    }
};

class vm
{
public:
    explicit vm(bvmc_instance* instance) noexcept : m_instance{instance} {}
    ~vm() noexcept { m_instance->destroy(m_instance); }

    vm(bvmc_instance* instance, std::initializer_list<std::pair<const char*, const char*>> options)
      : m_instance{instance}
    {
        for (auto option : options)
            set_option(option.first, option.second);
    }

    bool is_abi_compatible() const noexcept { return m_instance->abi_version == BVMC_ABI_VERSION; }

    char const* name() const noexcept { return m_instance->name; }

    char const* version() const noexcept { return m_instance->version; }

    bvmc_set_option_result set_option(const char name[], const char value[]) noexcept
    {
        return bvmc_set_option(m_instance, name, value);
    }

    result execute(bvmc_context& ctx,
                   bvmc_revision rev,
                   const bvmc_message& msg,
                   const uint8_t* code,
                   size_t code_size) noexcept
    {
        return result{m_instance->execute(m_instance, &ctx, rev, &msg, code, code_size)};
    }

private:
    bvmc_instance* const m_instance = nullptr;
};
}  // namespace bvmc
