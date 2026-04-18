#include "cubism_allocator.h"

#ifdef GDEXTENSION
#include <godot_cpp/core/memory.hpp>
#elif defined(GODOT_MODULE)
#include "core/os/memory.h"
#endif

void *CubismAllocator::Allocate(const Csm::csmSizeType size) {
	return memalloc(size);
}

void CubismAllocator::Deallocate(void *memory) {
	memfree(memory);
}

void *CubismAllocator::AllocateAligned(const Csm::csmSizeType size, const Csm::csmUint32 alignment) {
	Csm::csmSizeType offset, shift, alignedAddress;
	void *allocation;
	void **preamble;

	offset = alignment - 1 + sizeof(void *);

	allocation = Allocate(size + static_cast<Csm::csmUint32>(offset));

	alignedAddress = reinterpret_cast<Csm::csmSizeType>(allocation) + sizeof(void *);

	shift = alignedAddress % alignment;

	if (shift) {
		alignedAddress += (alignment - shift);
	}

	preamble = reinterpret_cast<void **>(alignedAddress);
	preamble[-1] = allocation;

	return reinterpret_cast<void *>(alignedAddress);
}

void CubismAllocator::DeallocateAligned(void *alignedMemory) {
	void **preamble;

	preamble = static_cast<void **>(alignedMemory);

	Deallocate(preamble[-1]);
}
