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
				Builder& SetImageFlag(VkImageCreateFlags flags);
				Builder& SetImageFormat(VkFormat format);
				Builder& SetImageUsage(VkImageUsageFlags usage);
				Builder& SetImageInitialLayout(VkImageLayout layout);
				Builder& SetImageViewAspectMask(VkImageAspectFlags aspectMask);
				Builder& SetMipLevels(int level)
				{
					imageCreateInfo.mipLevels = level;
					return *this;
				}
				Builder& SetArrayLayers(int count)
				{
					imageCreateInfo.arrayLayers = count;
					imageViewCreateInfo.subresourceRange.layerCount = static_cast<uint32_t>(count);
					return *this;
				}
				Builder& SetViewType(VkImageViewType type)
				{
					imageViewCreateInfo.viewType = type;
					return *this;
				}
				VkImageCreateInfo& GetImageCreateInfo() { return imageCreateInfo; }
				VkImageViewCreateInfo& GetImageViewCreateInfo() { return imageViewCreateInfo; }

				Image Build();
		};
	};
}

#endif // !IMAGE_H