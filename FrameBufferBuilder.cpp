#include "FrameBufferBuilder.h"
#include <stdexcept>

using namespace wl::backend;

FrameBufferBuilder::FrameBufferBuilder(VkDevice device, VkRenderPass renderPass, uint32_t width, uint32_t height)
{
	this->device = device;
	this->renderPass = renderPass;
	this->width = width;
	this->height = height;
}

FrameBufferBuilder& FrameBufferBuilder::AddAttachment(VkImageView imageView)
{
	attachments.push_back(imageView);
	return *this;
}

FrameBufferBuilder& wl::backend::FrameBufferBuilder::SetAttackments(std::vector<VkImageView>& imageViews)
{
	this->attachments = imageViews;
	return *this;
}

FrameBufferBuilder& FrameBufferBuilder::ClearAttachment()
{
	attachments.clear();
	return *this;
}

VkFramebuffer FrameBufferBuilder::Build()
{
	if (attachments.size() == 0)
	{

	}

	VkFramebuffer frameBuffer;
	VkFramebufferCreateInfo frameBufferInfo{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.width = this->width,
		.height = this->height,
		.layers = this->layers
	};

	if (vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &frameBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create framebuffer!");
	}

	return frameBuffer;
}