#pragma once
#ifndef WL_IMAGE_H
#define WL_IMAGE_H

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace wl::backend
{
	class Image
	{
	public:
		VkDevice device;
		VkImage vkImage;
		VkImageView vkImageView;

		VmaAllocator* vmaAllocator;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	public:
		~Image();
	private:
		Image() = default;

	public:
		class Builder
		{
			private:
				VkDevice device;
				VmaAllocator* vmaAllocator;
				VkImageCreateInfo imageCreateInfo;
				VkImageViewCreateInfo imageViewCreateInfo;

			public:
				Builder(VkDevice device, VmaAllocator& allocator, uint32_t width, uint32_t height);
				Builder SetImageFormat(VkFormat format);
				Builder SetImageUsage(VkImageUsageFlags usage);
				Builder SetImageLayout(VkImageLayout layout);
				Builder SetImageViewAspectMask(VkImageAspectFlags aspectMask);

				VkImageCreateInfo GetImageCreateInfo() const { return imageCreateInfo; }
				VkImageViewCreateInfo GetImageViewCreateInfo() const { return imageViewCreateInfo; }

				Image Build();
		};
	};
}

#endif // !IMAGE_H