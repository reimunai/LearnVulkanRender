#include "Image.h"
#include <stdexcept>
using namespace wl::backend;

Image::Builder::Builder(VkDevice device, VmaAllocator& allocator, uint32_t width, uint32_t height):
	vmaAllocator(&allocator), device(device)
{
	imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.extent{
			.width = width,
			.height = height,
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	imageViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = nullptr,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.subresourceRange
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		}
	};
}

Image::Builder Image::Builder::SetImageFormat(VkFormat format)
{
	imageCreateInfo.format = format;

	imageViewCreateInfo.format = format;
	return *this;
}

Image::Builder Image::Builder::SetImageUsage(VkImageUsageFlags usage)
{
	imageCreateInfo.usage = usage;
	return *this;
}

Image::Builder Image::Builder::SetImageLayout(VkImageLayout layout)
{
	imageCreateInfo.initialLayout = layout;
	return *this;
}

Image::Builder Image::Builder::SetImageViewAspectMask(VkImageAspectFlags aspectMask)
{
	imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	return *this;
}

Image Image::Builder::Build()
{
	Image image;

	VmaAllocationCreateInfo allocationCreateInfo{
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
	};

	if( vmaCreateImage(*vmaAllocator, &imageCreateInfo, &allocationCreateInfo, &image.vkImage, &image.allocation, &image.allocationInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}

	imageViewCreateInfo.image = image.vkImage;
	if(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &image.vkImageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}

	image.device = device;
	image.vmaAllocator = vmaAllocator;
	return image;
}

wl::backend::Image::~Image()
{
	vkDestroyImageView(device, vkImageView, nullptr);
	vmaDestroyImage(*vmaAllocator, vkImage, allocation);
}