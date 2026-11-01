#include "Buffer.h"
#include <stdexcept>
using namespace wl::v1::backend;

Buffer::~Buffer()
{
	if (allocator != nullptr && vkBuffer != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(allocator, vkBuffer, allocation);
	}
}
Buffer::Buffer()
{
	this->vkBuffer = VK_NULL_HANDLE;
	this->allocator = nullptr;
	this->allocation = nullptr;
	this->allocationInfo = {};
}
Buffer::Buffer(Buffer&& other) noexcept
{
	vkBuffer = other.vkBuffer;
	allocator = other.allocator;
	allocation = other.allocation;
	allocationInfo = other.allocationInfo;

	other.vkBuffer = VK_NULL_HANDLE;
	other.allocator = nullptr;
	other.allocation = nullptr;
	other.allocationInfo = {};
}

Buffer& wl::v1::backend::Buffer::operator=(Buffer&& other) noexcept
{
	this->vkBuffer = other.vkBuffer;
	this->allocator = other.allocator;
	this->allocation = other.allocation;
	this->allocationInfo = other.allocationInfo;

	other.vkBuffer = VK_NULL_HANDLE;
	other.allocator = nullptr;
	other.allocation = nullptr;
	other.allocationInfo = {};

	return *this;
}

//Host allocatable buffer, copy data from CPU to GPU
void Buffer::CopyDataToBuffer(void* data)
{
	if (allocator == nullptr || allocation == nullptr) {
		throw std::runtime_error("Buffer::CopyDataToBuffer: allocator or allocation is null.");
	}
	size_t size =  allocationInfo.size;
	if (size == 0) {
		throw std::runtime_error("Buffer::CopyDataToBuffer: allocation size is zero.");
	}
	void* pData = nullptr;
	VkResult mapRes = vmaMapMemory(allocator, allocation, &pData);
	if (mapRes != VK_SUCCESS || pData == nullptr) {
		throw std::runtime_error("Buffer::CopyDataToBuffer: vmaMapMemory failed.");
	}
	memcpy(pData, data, size);
	vmaFlushAllocation(allocator, allocation, 0, size);
	vmaUnmapMemory(allocator, allocation);
}

Buffer::Builder::Builder(VmaAllocator& allocator, uint64_t size, VkBufferUsageFlags bufferUsage) :
	allocator(allocator), size(size), bufferUsage(bufferUsage) 
{
	memoryFLag = static_cast<VmaAllocationCreateFlagBits>(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
	memoryUsage = VMA_MEMORY_USAGE_AUTO;
}
Buffer::Builder& Buffer::Builder::SetBufferUsage(VkBufferUsageFlags usage)
{
	this->bufferUsage = usage;
	return *this;
}
Buffer::Builder& Buffer::Builder::SetSize(uint64_t size)
{
	this->size = size;
	return *this;
}

Buffer::Builder& Buffer::Builder::SetMemoryFlag(VmaAllocationCreateFlags memoryFLag)
{
	this->memoryFLag = memoryFLag;
	return *this;
}

Buffer::Builder& Buffer::Builder::SetMemoryUsage(VmaMemoryUsage memoryUsage)
{
	this->memoryUsage = memoryUsage;
	return *this;
}

Buffer Buffer::Builder::Build()
{
	Buffer buffer;
	buffer.allocator = allocator;

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