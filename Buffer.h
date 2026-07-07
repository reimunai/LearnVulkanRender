#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"	

namespace wl::v1::backend
{
	class Buffer
	{
	public:
		VkBuffer vkBuffer;

		//uint64_t size;
		VmaAllocator* allocator;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;

	public:
		~Buffer();
		void CopyDataToBuffer(void* data);
	private:
		Buffer();

	public:
		class Builder
		{
		private:
			VmaAllocator& allocator;
			uint64_t size;
			VkBufferUsageFlagBits bufferUsage;

			VmaAllocationCreateFlagBits memoryFLag;
			VmaMemoryUsage memoryUsage;

		public:
			Builder(VmaAllocator& allocator, uint64_t size, VkBufferUsageFlagBits bufferUsage);
			Buffer::Builder SetBufferUsage(VkBufferUsageFlagBits usage);
			Buffer::Builder SetSize(uint64_t size);
			Buffer::Builder SetMemoryFlag(VmaAllocationCreateFlagBits memoryFLag);
			Buffer::Builder SetMemoryUsage(VmaMemoryUsage memoryUsage);
			Buffer Build();
		};
	};
}

#endif // !BUFFER_H
