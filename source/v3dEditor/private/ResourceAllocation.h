#pragma once

namespace ve {

	class ResourceMemory;

	struct ResourceAllocationT
	{
		ResourceMemory* pOwner;

		bool exist;
		bool used;

		uint64_t offset;
		uint64_t size;

		ResourceAllocationT* pPrev;
		ResourceAllocationT* pNext;

		VE_DECLARE_ALLOCATOR
	};

}