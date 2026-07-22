#include "CubeImageFactor.h"
#include <stb_image.h>
#include <array>
#include <iostream>

#include "Buffer.h"

wl::backend::Image wl::resource::CubeImageFactor::CreateCubeImage(VkDevice device, VmaAllocator allocator, uint32_t width, uint32_t height)
{
	using wl::backend::Image;
	
	return	Image::Builder(device, allocator, width, height)
			.SetImageFlag(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
			.SetImageFormat(VK_FORMAT_R8G8B8A8_SRGB)
			.SetImageUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.SetImageInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
			.SetImageViewAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.SetArrayLayers(6)
			.SetViewType(VK_IMAGE_VIEW_TYPE_CUBE)
			.Build();
}

void wl::resource::CubeImageFactor::CopyTextureToCubeImage(wl::backend::Image& cubeImage, std::vector<std::string>& filepaths, VmaAllocator allocator, wl::backend::VkBackend& backend)
{
	using wl::v1::backend::Buffer;
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filepaths[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	Buffer tbuffer =
		Buffer::Builder(allocator, 6 * imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
		.Build();

	void* pData = nullptr;
	vmaMapMemory(tbuffer.allocator, tbuffer.allocation, &pData);
	memcpy(pData, pixels, imageSize);

	for (int i = 1; i < 6; i++)
	{
		pixels = stbi_load(filepaths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		if (!pixels)
		{
			throw std::runtime_error("failed to load texture image!");
		}
		memcpy((uint8_t*)pData + imageSize * i, pixels, imageSize);
	}
	vmaFlushAllocation(tbuffer.allocator, tbuffer.allocation, 0, 6 * imageSize);
	vmaUnmapMemory(tbuffer.allocator, tbuffer.allocation);
	stbi_image_free(pixels);

	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

		.image = cubeImage.vkImage,
		.subresourceRange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 6,
		},
	};
	auto cmd = backend.BeginOneTimesCommands();
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	std::array<VkBufferImageCopy, 6>copyRegion{};
	for (int i = 0; i < 6; i++)
	{
		copyRegion[i] = {
			.bufferOffset = i * imageSize,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,

			.imageSubresource{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = static_cast<uint32_t>(i),
				.layerCount = 1
			},

			.imageOffset = {0, 0, 0},
			.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1}
		};
	}
	vkCmdCopyBufferToImage(cmd, tbuffer.vkBuffer, cubeImage.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, copyRegion.data());

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	backend.EndOneTimesCommands(cmd);
}
