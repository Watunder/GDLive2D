#pragma once

#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>

class CubismAllocator : public Csm::ICubismAllocator {
public:
	virtual void *Allocate(const Csm::csmSizeType size) override;
	virtual void Deallocate(void *memory) override;
	virtual void *AllocateAligned(const Csm::csmSizeType size, const Csm::csmUint32 alignment) override;
	virtual void DeallocateAligned(void *alignedMemory) override;
};
