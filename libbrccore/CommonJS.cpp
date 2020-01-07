#include "CommonJS.h"
#include <sstream>
#include <string>
#include <libbrccore/Exceptions.h>
#include <libdevcrypto/base58.h>
namespace dev
{
    using h192 = FixedHash<24>;

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

std::string to2HashAddress(Address const &_addr) {
    return toHex(sha3(sha3(Address(_addr))).ref().cropped(0, 4).toBytes());
}

Address jsToAddressFromNewAddress(std::string const &_ns) {
    try {
        ///check length and brc
        if (_ns.length() <=3 || _ns.substr(0, 3) != "brc") {
            BOOST_THROW_EXCEPTION(dev::brc::InvalidAddress());
        }
        auto base58Str = _ns.substr(3, _ns.length());
        auto hash_addr = dev::crypto::from_base58(base58Str);
        if (hash_addr.size() != 24) {
            BOOST_THROW_EXCEPTION(dev::brc::InvalidAddress());
        }
        auto hash_addr_str = toHex(hash_addr);
        ///check addr hash
        auto old_addr = hash_addr_str.substr(8);
        if (hash_addr_str.substr(0, 8) != to2HashAddress(Address(old_addr))) {
            BOOST_THROW_EXCEPTION(dev::brc::InvalidAddress());
        }
        return jsToAddress(old_addr);
    }
    catch (dev::crypto::Base58_decode_exception& e){}
    catch (dev::brc::InvalidAddress){}
    catch (std::exception e){}
    catch (...){}
    BOOST_THROW_EXCEPTION(dev::brc::InvalidAddress());
}

std::string jsToNewAddress(Address const& _addr) {
    //Address _addr = jsToAddress(_s);
    std::string new_addr = "brc";
    try {
        auto newAddress = to2HashAddress(_addr) + toHex(_addr);
        auto hexAddress = h192(newAddress);
        if(hexAddress == h192())
            BOOST_THROW_EXCEPTION(dev::brc::InvalidAddress());
        return "brc" + dev::crypto::to_base58((char*)hexAddress.data(), 24);
    }
    catch (dev::crypto::Base58_decode_exception){}
    catch (std::exception){}
    catch (...){}
    BOOST_THROW_EXCEPTION(dev::brc::InvalidAddress());
}

Address jsToAddressAcceptAllAddress(std::string const &_allAddr) {
    if(_allAddr.empty() || _allAddr=="0x")
        return Address();
    try{
        return jsToAddressFromNewAddress(_allAddr);
    }
    catch (...){}
    try {
        return jsToAddress(_allAddr);
    }
    catch (...){}
    BOOST_THROW_EXCEPTION(dev::brc::InvalidAddress());
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
