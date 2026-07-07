#include "Buffer.h"
#include <stdexcept>
using namespace wl::v1::backend;

Buffer::~Buffer()
{
	vmaDestroyBuffer(*allocator, vkBuffer, allocation);
}

Buffer::Buffer(Buffer&& other)
{
	vkBuffer = other.vkBuffer;
	allocator = other.allocator;
	allocation = other.allocation;
	allocationInfo = other.allocationInfo;

	other.vkBuffer = nullptr;
	other.allocator = nullptr;
	other.allocation = nullptr;
}

//Host allocatable buffer, copy data from CPU to GPU
void Buffer::CopyDataToBuffer(void* data)
{
	size_t size =  allocationInfo.size;
	void* pData;
	vmaMapMemory(*allocator, allocation, &pData);
	memcpy(pData, data, size);
	vmaFlushAllocation(*allocator, allocation, 0, size);
	vmaUnmapMemory(*allocator, allocation);
}

Buffer::Builder::Builder(VmaAllocator& allocator, uint64_t size, VkBufferUsageFlagBits bufferUsage) :
	allocator(allocator), size(size), bufferUsage(bufferUsage) 
{
	memoryFLag = static_cast<VmaAllocationCreateFlagBits>(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
	memoryUsage = VMA_MEMORY_USAGE_AUTO;
}
Buffer::Builder Buffer::Builder::SetBufferUsage(VkBufferUsageFlagBits usage)
{
	bufferUsage = usage;
	return *this;
}
Buffer::Builder Buffer::Builder::SetSize(uint64_t size)
{
	this->size = size;
	return *this;
}

Buffer::Builder Buffer::Builder::SetMemoryFlag(VmaAllocationCreateFlagBits memoryFLag)
{
	this->memoryFLag = memoryFLag;
	return *this;
}

Buffer::Builder Buffer::Builder::SetMemoryUsage(VmaMemoryUsage memoryUsage)
{
	this->memoryUsage = memoryUsage;
	return *this;
}

Buffer Buffer::Builder::Build()
{
	Buffer buffer;
	buffer.allocator = &allocator;

	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = static_cast<VkBufferUsageFlags>(bufferUsage),
	};
	VmaAllocationCreateInfo allocationCreateInfo{
		.flags = static_cast<VmaAllocationCreateFlags>(memoryFLag),
		.usage = memoryUsage,
	};
	if (vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer.vkBuffer, &buffer.allocation, &buffer.allocationInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer!");
	}
	return buffer;
}