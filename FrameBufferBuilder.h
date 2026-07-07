#pragma once
#ifndef FRAME_BUFFER_BUILDER_H

#include "vulkan/vulkan.h"
#include <vector>
namespace wl::backend
{
	class FrameBufferBuilder
	{
	private:
		VkDevice device;
		VkRenderPass renderPass;
		std::vector<VkImageView> attachments;
		uint32_t width;
		uint32_t height;
		uint32_t layers = 1;
	public:
		FrameBufferBuilder(VkDevice device, VkRenderPass renderPass, uint32_t width, uint32_t height);
		FrameBufferBuilder AddAttachment(VkImageView imageView);
		FrameBufferBuilder SetAttackments(std::vector<VkImageView>& imageViews);
		FrameBufferBuilder ClearAttachment();
		VkFramebuffer Build();
	};
}

#endif // !FRAME_BUFFER_BUILDER_H
