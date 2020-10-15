#pragma once

#include <libbvm/VMFace.h>

namespace dev
{
namespace brc
{
/// The wrapper implementing the VMFace interface with a BVMC VM as a backend.
class BVMC : public bvmc::VM, public VMFace
{
public:
    explicit BVMC(bvmc_vm* _vm) noexcept;

    owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) final;
};
}  // namespace brc
}  // namespace dev
