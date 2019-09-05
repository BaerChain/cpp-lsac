#include "CommonJS.h"
#include <sstream>
#include <string>
#include <libbrccore/Exceptions.h>
namespace dev
{
std::string prettyU256(u256 _n, bool _abridged)
{
    std::string raw;
    std::ostringstream s;
    if (!(_n >> 64))
        s << " " << (uint64_t)_n << " (0x" << std::hex << (uint64_t)_n << ")";
    else if (!~(_n >> 64))
        s << " " << (int64_t)_n << " (0x" << std::hex << (int64_t)_n << ")";
    else if ((_n >> 160) == 0)
    {
        Address a = right160(_n);

        std::string n;
        if (_abridged)
            n = a.abridged();
        else
            n = toHex(a.ref());

        if (n.empty())
            s << "0";
        else
            s << _n << "(0x" << n << ")";
    }
    else if (!(raw = fromRaw((h256)_n)).empty())
        return "\"" + raw + "\"";
    else
        s << "" << (h256)_n;
    return s.str();
}

namespace brc
{

BlockNumber jsToBlockNumber(std::string const& _js)
{
    std::cout << "123" << std::endl;
    return LatestBlock;
}

BlockNumber jsToBlockNum(std::string const& _js)
{
    if (_js == "latest")
    {
        return LatestBlock;
    }else if (_js == "earliest")
    {
        return 0;
    }else if (_js == "pending")
    {
        return PendingBlock;
    }else
    {
        int i = jsToInt(_js);
        if(i <= 0)
        {
            CHECK_RPC_THROW_INFO("_blockNumber must: latest, earliest, pending or larger -1.")
        }
        return (unsigned)i;
    }
}

uint8_t jsToOrderEnum(std::string const& _js)
{
    if (_js == "sell")
    {
        return 1;
    }
    else if (_js == "buy")
    {
        return 2;
    }
    else if (_js == "FUEL")
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int64_t jsToint64(std::string const& _js)
{
    std::istringstream str(_js);
    int64_t num;
    str >> num;
    return num;
}

}  // namespace brc

}  // namespace dev
