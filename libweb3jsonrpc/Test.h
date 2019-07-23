#pragma once
#include "TestFace.h"

namespace dev
{

namespace brc
{
class Client;
}

namespace rpc
{

class Test: public TestFace
{
public:
    Test(brc::Client& _brc);
    virtual RPCModules implementedModules() const override
    {
        return RPCModules{RPCModule{"test", "1.0"}};
    }
    virtual std::string test_getLogHash(std::string const& _param1) override;
    virtual std::string test_importRawBlock(std::string const& _blockRLP) override;
    virtual bool test_setChainParams(const Json::Value& _param1) override;
    virtual bool test_mineBlocks(int _number) override;
    virtual bool test_modifyTimestamp(int _timestamp) override;
    virtual bool test_rewindToBlock(int _number) override;

private:
    brc::Client& m_brc;
};

}
}
