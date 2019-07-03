#include "BrchashProofOfWork.h"
#include "Brchash.h"
using namespace std;
using namespace chrono;
using namespace dev;
using namespace brc;

const unsigned BrchashProofOfWork::defaultLocalWorkSize = 64;
const unsigned BrchashProofOfWork::defaultGlobalWorkSizeMultiplier = 4096; // * CL_DEFAULT_LOCAL_WORK_SIZE
const unsigned BrchashProofOfWork::defaultMSPerBatch = 0;
const BrchashProofOfWork::WorkPackage BrchashProofOfWork::NullWorkPackage = BrchashProofOfWork::WorkPackage();

BrchashProofOfWork::WorkPackage::WorkPackage(BlockHeader const& _bh):
	boundary(Brchash::boundary(_bh)),
	seedHash(Brchash::seedHash(_bh)),
	m_headerHash(_bh.hash(WithoutSeal))
{}

BrchashProofOfWork::WorkPackage::WorkPackage(WorkPackage const& _other):
	boundary(_other.boundary),
	seedHash(_other.seedHash),
	m_headerHash(_other.headerHash())
{}

BrchashProofOfWork::WorkPackage& BrchashProofOfWork::WorkPackage::operator=(BrchashProofOfWork::WorkPackage const& _other)
{
	if (this == &_other)
		return *this;
	boundary = _other.boundary;
	seedHash = _other.seedHash;
	h256 headerHash = _other.headerHash();
	{
		Guard l(m_headerHashLock);
		m_headerHash = std::move(headerHash);
	}
	return *this;
}
