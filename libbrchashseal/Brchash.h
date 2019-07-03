#pragma once

#include <libbrccore/SealEngine.h>
#include <libbrcdchain/GenericFarm.h>
#include <libbrcdchain/Client.h>
#include "BrchashProofOfWork.h"

#include <brcash/brcash.hpp>

#include <chrono>

namespace dev
{

namespace brc
{
inline brcash::hash256 toBrchash(h256 const& hash) noexcept
{
    std::cout << "Brchash::toBrchash" << std::endl;
    return brcash::hash256_from_bytes(hash.data());
}

inline uint64_t toBrchash(Nonce const& nonce) noexcept
{
    std::cout << "Brchash::toBrchash" << std::endl;
    return static_cast<uint64_t>(static_cast<u64>(nonce));
}

class Brchash: public SealEngineBase
{
public:
    Brchash();
    ~Brchash();

    static std::string name() {  return "Brchash"; }
    unsigned revision() const override { std::cout << "Brchash::revision" << std::endl; return 1; }
    unsigned sealFields() const override { std::cout << "Brchash::sealFields" << std::endl; return 2; }
    bytes sealRLP() const override { std::cout << "Brchash::sealRLP" << std::endl; return rlp(h256()) + rlp(Nonce()); }

    StringHashMap jsInfo(BlockHeader const& _bi) const override;
    void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const override;
    void verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, BlockHeader const& _header, u256 const& _startGasUsed) const override;
    void populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const override;

    strings sealers() const override;
    std::string sealer() const override { std::cout << "Brchash::sealer" << std::endl; return m_sealer; }
    void setSealer(std::string const& _sealer) override { m_sealer = _sealer; }
    void cancelGeneration() override { m_farm.stop(); }
    void generateSeal(BlockHeader const& _bi) override;
    bool shouldSeal(Interface* _i) override;

    brc::GenericFarm<BrchashProofOfWork>& farm() { std::cout << "Brchash::farm" << std::endl; return m_farm; }

    static h256 seedHash(BlockHeader const& _bi);
    static Nonce nonce(BlockHeader const& _bi) { std::cout << "Brchash::nonce" << std::endl; return _bi.seal<Nonce>(NonceField); }
    static h256 mixHash(BlockHeader const& _bi) { std::cout << "Brchash::mixHash" << std::endl; return _bi.seal<h256>(MixHashField); }

    static h256 boundary(BlockHeader const& _bi)
    {
        std::cout << "Brchash::boundary" << std::endl; 
        auto const& d = _bi.difficulty();
        return d > 1 ? h256{u256((u512(1) << 256) / d)} : ~h256{};
    }

    static BlockHeader& setNonce(BlockHeader& _bi, Nonce _v) { std::cout << "Brchash::setNonce" << std::endl; _bi.setSeal(NonceField, _v); return _bi; }
    static BlockHeader& setMixHash(BlockHeader& _bi, h256 const& _v) { std::cout << "Brchash::setMixHash" << std::endl; _bi.setSeal(MixHashField, _v); return _bi; }

    void manuallySetWork(BlockHeader const& _work) { m_sealing = _work; }
    void manuallySubmitWork(h256 const& _mixHash, Nonce _nonce);

    static void init();

private:
    bool verifySeal(BlockHeader const& _blockHeader) const;
    bool quickVerifySeal(BlockHeader const& _blockHeader) const;

    brc::GenericFarm<BrchashProofOfWork> m_farm;
    std::string m_sealer = "cpu";
    BlockHeader m_sealing;

    /// A mutex covering m_sealing
    Mutex m_submitLock;
};




class Dpos: public brc::SealEngineBase
{
public:
    static std::string name() { return "Dpos"; }
    static void init();
    void generateSeal(BlockHeader const& _bi) override;
private:
    std::chrono::high_resolution_clock::time_point m_lasttimesubmit;
};



class DposClient: public Client
{
public:
    DposClient(
	    ChainParams const& _params, 
        int _networkID, 
        p2p::Host& _host,
        std::shared_ptr<GasPricer> _gpForAdoption, 
        boost::filesystem::path const& _dbPath = {},
        boost::filesystem::path const& _snapshotPath = {},
        WithExisting _forceAction = WithExisting::Trust,
        TransactionQueue::Limits const& _l = TransactionQueue::Limits{2048, 2048}
	);
};



/*
inline brcash::hash256 toBrchash(h256 const& hash) noexcept
{
    return brcash::hash256_from_bytes(hash.data());
}

inline uint64_t toBrchash(Nonce const& nonce) noexcept
{
    return static_cast<uint64_t>(static_cast<u64>(nonce));
}

class Dpos: public SealEngineBase
{
public:
    Dpos();
    ~Dpos();

    static std::string name() { return "Dpos"; }
    unsigned revision() const override { return 1; }
    unsigned sealFields() const override { return 2; }
    bytes sealRLP() const override { return rlp(h256()) + rlp(Nonce()); }

    StringHashMap jsInfo(BlockHeader const& _bi) const override;
    void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const override;
    void verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, BlockHeader const& _header, u256 const& _startGasUsed) const override;
    void populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const override;

    strings sealers() const override;
    std::string sealer() const override { return m_sealer; }
    void setSealer(std::string const& _sealer) override { m_sealer = _sealer; }
    void cancelGeneration() override { m_farm.stop(); }
    void generateSeal(BlockHeader const& _bi) override;
    bool shouldSeal(Interface* _i) override;

    brc::GenericFarm<BrchashProofOfWork>& farm() { return m_farm; }

    static h256 seedHash(BlockHeader const& _bi);
    static Nonce nonce(BlockHeader const& _bi) { return _bi.seal<Nonce>(NonceField); }
    static h256 mixHash(BlockHeader const& _bi) { return _bi.seal<h256>(MixHashField); }

    static h256 boundary(BlockHeader const& _bi)
    {
        auto const& d = _bi.difficulty();
        return d > 1 ? h256{u256((u512(1) << 256) / d)} : ~h256{};
    }

    static BlockHeader& setNonce(BlockHeader& _bi, Nonce _v) { _bi.setSeal(NonceField, _v); return _bi; }
    static BlockHeader& setMixHash(BlockHeader& _bi, h256 const& _v) { _bi.setSeal(MixHashField, _v); return _bi; }

    void manuallySetWork(BlockHeader const& _work) { m_sealing = _work; }
    void manuallySubmitWork(h256 const& _mixHash, Nonce _nonce);

    static void init();

private:
    bool verifySeal(BlockHeader const& _blockHeader) const;
    bool quickVerifySeal(BlockHeader const& _blockHeader) const;

    brc::GenericFarm<BrchashProofOfWork> m_farm;
    std::string m_sealer = "cpu";
    BlockHeader m_sealing;

    /// A mutex covering m_sealing
    Mutex m_submitLock;
};
*/








}
}
