#include "Brchash.h"
#include "BrchashCPUMiner.h"

#include <libbrccore/ChainOperationParams.h>
#include <libbrccore/CommonJS.h>
#include <libbrcdchain/Interface.h>

#include <brcash/brcash.hpp>

using namespace std;
using namespace dev;
using namespace brc;

void Brchash::init() {
    BRC_REGISTER_SEAL_ENGINE(Brchash);
}

Brchash::Brchash() {
    std::cout << "Brchash::Brchash" << std::endl;
    map<string, GenericFarm<BrchashProofOfWork>::SealerDescriptor> sealers;
    sealers["cpu"] = GenericFarm<BrchashProofOfWork>::SealerDescriptor{&BrchashCPUMiner::instances,
                                                                      [](GenericMiner<BrchashProofOfWork>::ConstructionInfo ci) {
                                                                          return new BrchashCPUMiner(ci);
                                                                      }};
    m_farm.setSealers(sealers);
    m_farm.onSolutionFound([=](BrchashProofOfWork::Solution const &sol) {
        std::unique_lock<Mutex> l(m_submitLock);
//        cdebug << m_farm.work().seedHash << m_farm.work().headerHash << sol.nonce << BrchashAux::eval(m_farm.work().seedHash, m_farm.work().headerHash, sol.nonce).value;
        setMixHash(m_sealing, sol.mixHash);
        setNonce(m_sealing, sol.nonce);
        if (!quickVerifySeal(m_sealing))
            return false;

        if (m_onSealGenerated) {
            RLPStream ret;
            m_sealing.streamRLP(ret);
            l.unlock();
            m_onSealGenerated(ret.out());
        }
        return true;
    });
}

Brchash::~Brchash() {
    std::cout << "Brchash::~Brchash" << std::endl;
    // onSolutionFound closure sometimes has references to destroyed members.
    m_farm.onSolutionFound({});
}

strings Brchash::sealers() const {
    std::cout << "Brchash::Brchash" << std::endl;
    return {"cpu"};
}

h256 Brchash::seedHash(BlockHeader const &_bi) {
    std::cout << "Brchash::seedHash" << std::endl;
    unsigned epoch = static_cast<unsigned>(_bi.number()) / BRCASH_EPOCH_LENGTH;

    h256 seed;
    for (unsigned n = 0; n < epoch; ++n)
        seed = sha3(seed);
    return seed;
}

StringHashMap Brchash::jsInfo(BlockHeader const &_bi) const {
    std::cout << "Brchash::jsInfo" << std::endl;
    return {{"nonce",      toJS(nonce(_bi))},
            {"seedHash",   toJS(seedHash(_bi))},
            {"mixHash",    toJS(mixHash(_bi))},
            {"boundary",   toJS(boundary(_bi))},
            {"difficulty", toJS(_bi.difficulty())}};
}

void Brchash::verify(Strictness _s, BlockHeader const &_bi, BlockHeader const &_parent, bytesConstRef _block) const {
    std::cout << "Brchash::verify" << std::endl;
    SealEngineFace::verify(_s, _bi, _parent, _block);

    if (_parent) {
        // Check difficulty is correct given the two timestamps.
        auto expected = calculateBrchashDifficulty(chainParams(), _bi, _parent);
        auto difficulty = _bi.difficulty();
        if (difficulty != expected)
            BOOST_THROW_EXCEPTION(InvalidDifficulty() << RequirementError((bigint) expected, (bigint) difficulty));
    }

    // check it hashes according to proof of work or that it's the genesis block.
    if (_s == CheckEverything && _bi.parentHash() && !verifySeal(_bi)) {
        brcash::result result =
                brcash::hash(brcash::get_global_epoch_context(brcash::get_epoch_number(_bi.number())),
                             toBrchash(_bi.hash(WithoutSeal)), toBrchash(nonce(_bi)));

        h256 mix{result.mix_hash.bytes, h256::ConstructFromPointer};
        h256 final{result.final_hash.bytes, h256::ConstructFromPointer};

        InvalidBlockNonce ex;
        ex << errinfo_nonce(nonce(_bi));
        ex << errinfo_mixHash(mixHash(_bi));
        ex << errinfo_seedHash(seedHash(_bi));
        ex << errinfo_brcashResult(make_tuple(final, mix));
        ex << errinfo_hash256(_bi.hash(WithoutSeal));
        ex << errinfo_difficulty(_bi.difficulty());
        ex << errinfo_target(boundary(_bi));
        BOOST_THROW_EXCEPTION(ex);
    } else if (_s == QuickNonce && _bi.parentHash() && !quickVerifySeal(_bi)) {
        InvalidBlockNonce ex;
        ex << errinfo_hash256(_bi.hash(WithoutSeal));
        ex << errinfo_difficulty(_bi.difficulty());
        ex << errinfo_nonce(nonce(_bi));
        BOOST_THROW_EXCEPTION(ex);
    }
}

void Brchash::verifyTransaction(ImportRequirements::value _ir, TransactionBase const &_t, BlockHeader const &_header,
                               u256 const &_startGasUsed) const {
    std::cout << "Brchash::verifyTransaction" << std::endl;
    SealEngineFace::verifyTransaction(_ir, _t, _header, _startGasUsed);

    if (_ir & ImportRequirements::TransactionSignatures) {
        if (_header.number() >= chainParams().EIP158ForkBlock) {
            int chainID = chainParams().chainID;
            _t.checkChainId(chainID);
        } else
            _t.checkChainId(-4);
    }
}

void Brchash::manuallySubmitWork(const h256 &_mixHash, Nonce _nonce) {
    std::cout << "Brchash::manuallySubmitWork" << std::endl;
    m_farm.submitProof(BrchashProofOfWork::Solution{_nonce, _mixHash}, nullptr);
}

void Brchash::populateFromParent(BlockHeader &_bi, BlockHeader const &_parent) const {
    std::cout << "Brchash::populateFromParent" << std::endl;
    SealEngineFace::populateFromParent(_bi, _parent);
    _bi.setDifficulty(calculateBrchashDifficulty(chainParams(), _bi, _parent));
    _bi.setGasLimit(calculateGasLimit(chainParams(), _bi));
}

bool Brchash::quickVerifySeal(BlockHeader const &_blockHeader) const {
    std::cout << "Brchash::quickVerifySeal" << std::endl;
    h256 const h = _blockHeader.hash(WithoutSeal);
    h256 const b = boundary(_blockHeader);
    Nonce const n = nonce(_blockHeader);
    h256 const m = mixHash(_blockHeader);

    return brcash::verify_final_hash(toBrchash(h), toBrchash(m), toBrchash(n), toBrchash(b));
}

bool Brchash::verifySeal(BlockHeader const &_blockHeader) const {
    std::cout << "Brchash::verifySeal" << std::endl;
    h256 const h = _blockHeader.hash(WithoutSeal);
    h256 const b = boundary(_blockHeader);
    Nonce const n = nonce(_blockHeader);
    h256 const m = mixHash(_blockHeader);

    auto &context =
            brcash::get_global_epoch_context(brcash::get_epoch_number(_blockHeader.number()));
    return brcash::verify(context, toBrchash(h), toBrchash(m), toBrchash(n), toBrchash(b));
}

void Brchash::generateSeal(BlockHeader const &_bi) {
    std::cout << "Brchash::generateSeal" << std::endl;
    Guard l(m_submitLock);
    m_sealing = _bi;
    m_farm.setWork(m_sealing);
    m_farm.start(m_sealer);
    m_farm.setWork(m_sealing);
}

bool Brchash::shouldSeal(Interface *) {
    std::cout << "Brchash::shouldSeal" << std::endl;
    return true;
}


/*void Dpos::init() {
    BRC_REGISTER_SEAL_ENGINE(Dpos);
}

void Dpos::generateSeal(BlockHeader const &_bi) {
    chrono::high_resolution_clock::time_point v_timeNow = chrono::high_resolution_clock::now();
    chrono::duration<double, std::ratio<1, 1000>> duration_ms = chrono::duration_cast<chrono::duration<double, std::ratio<1, 1000>>>(
            v_timeNow - m_lasttimesubmit);
    BlockHeader header(_bi);
    header.setSeal(NonceField, h64{0});
    header.setSeal(MixHashField, h256{0});
    RLPStream ret;
    header.streamRLP(ret);
    std::cout << "dpos generateSeal:" << std::endl;
    if (m_onSealGenerated && duration_ms.count() > 10000) {
        m_onSealGenerated(ret.out());
        m_lasttimesubmit = chrono::high_resolution_clock::now();
    }
}*/

/*SHDposClient::SHDposClient(
        ChainParams const &_params,
        int _networkID,
        p2p::Host &_host,
        std::shared_ptr<GasPricer> _gpForAdoption,
        boost::filesystem::path const &_dbPath,
        boost::filesystem::path const &_snapshotPath,
        WithExisting _forceAction,
        TransactionQueue::Limits const &_limits)
        : Client(
        _params, _networkID, _host, _gpForAdoption, _dbPath, _snapshotPath, _forceAction, _limits) {
    // will throw if we're not an Brchash seal engine.
    //asBrchashClient(*this);
}
*/


/*
void Dpos::init()
{
    BRC_REGISTER_SEAL_ENGINE(Dpos);
}

Dpos::Dpos()
{
    
}

Dpos::~Dpos()
{
    // onSolutionFound closure sometimes has references to destroyed members.
    m_farm.onSolutionFound({});
}

strings Dpos::sealers() const
{
    return {"cpu"};
}

h256 Dpos::seedHash(BlockHeader const& _bi)
{

    unsigned epoch = static_cast<unsigned>(_bi.number()) / BRCASH_EPOCH_LENGTH;

    h256 seed;
    for (unsigned n = 0; n < epoch; ++n)
        seed = sha3(seed);
    return seed;
}

StringHashMap Dpos::jsInfo(BlockHeader const& _bi) const
{
    return { { "nonce", toJS(nonce(_bi)) }, { "seedHash", toJS(seedHash(_bi)) }, { "mixHash", toJS(mixHash(_bi)) }, { "boundary", toJS(boundary(_bi)) }, { "difficulty", toJS(_bi.difficulty()) } };
}

void Dpos::verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const
{
    SealEngineFace::verify(_s, _bi, _parent, _block);

    
}

void Dpos::verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, BlockHeader const& _header, u256 const& _startGasUsed) const
{
    SealEngineFace::verifyTransaction(_ir, _t, _header, _startGasUsed);

    
}

void Dpos::manuallySubmitWork(const h256& _mixHash, Nonce _nonce)
{
    m_farm.submitProof(BrchashProofOfWork::Solution{_nonce, _mixHash}, nullptr);
}

void Dpos::populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const
{
    SealEngineFace::populateFromParent(_bi, _parent);
    _bi.setDifficulty(calculateBrchashDifficulty(chainParams(), _bi, _parent));
    _bi.setGasLimit(calculateGasLimit(chainParams(), _bi));
}

bool Dpos::quickVerifySeal(BlockHeader const& _blockHeader) const
{
    h256 const h = _blockHeader.hash(WithoutSeal);
    h256 const b = boundary(_blockHeader);
    Nonce const n = nonce(_blockHeader);
    h256 const m = mixHash(_blockHeader);

    return brcash::verify_final_hash(toBrchash(h), toBrchash(m), toBrchash(n), toBrchash(b));
}

bool Dpos::verifySeal(BlockHeader const& _blockHeader) const
{
    h256 const h = _blockHeader.hash(WithoutSeal);
    h256 const b = boundary(_blockHeader);
    Nonce const n = nonce(_blockHeader);
    h256 const m = mixHash(_blockHeader);

    auto& context =
        brcash::get_global_epoch_context(brcash::get_epoch_number(_blockHeader.number()));
    return brcash::verify(context, toBrchash(h), toBrchash(m), toBrchash(n), toBrchash(b));
}

void Dpos::generateSeal(BlockHeader const& _bi)
{
    Guard l(m_submitLock);
    m_sealing = _bi;
    m_farm.setWork(m_sealing);
    m_farm.start(m_sealer);
    m_farm.setWork(m_sealing);
}

bool Dpos::shouldSeal(Interface*)
{
    return true;
}
*/

