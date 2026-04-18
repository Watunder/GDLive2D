#pragma once

#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>

class CubismAllocator : public Csm::ICubismAllocator {
public:
	/**
	 * Allocates the memory.
	 *
	 * @param size Desired amount of memory in bytes
	 *
	 * @return Pointer to the allocated memory if succeeded; otherwise `0`
	 */
	virtual void *Allocate(const Csm::csmSizeType size) override;

	/**
	 * Deallocates the memory.
	 *
	 * @param memory Pointer to allocated memory to be deallocated
	 */
	virtual void Deallocate(void *memory) override;

	/**
	 * Allocates the memory with specified alignment.
	 *
	 * @param size Desired amount of memory in bytes
	 * @param alignment Desired alignment of memory in bytes
	 *
	 * @return Pointer to the allocated memory if succeeded; otherwise `0`
	 */
	virtual void *AllocateAligned(const Csm::csmSizeType size, const Csm::csmUint32 alignment) override;

	/**
	 * Deallocates the aligned memory.
	 *
	 * @param alignedMemory Pointer to allocated memory to be deallocated
	 */
	virtual void DeallocateAligned(void *alignedMemory) override;
};
