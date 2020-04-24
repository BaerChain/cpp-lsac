#pragma once
namespace dev
{
namespace brc
{
class SHDposHostCapability;

class SHDposSync
{
public:
    SHDposSync(SHDposHostCapability& host) : m_host(host) {}

private:
    SHDposHostCapability& m_host;


    uint32_t lastRequestNumber = 0;
    uint32_t lastImportedNumber = 0;
};
}  // namespace brc
}  // namespace dev