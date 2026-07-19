#pragma once
#ifndef RENDER_PASS_BUILDER_H
#define RENDER_PASS_BUILDER_H


#include "vulkan/vulkan.h"
#include <vector>

namespace wl::backend
{
	class RenderPassBuilder
	{
	private:
		VkDevice device;

		std::vector<VkAttachmentDescription> Attachments;
		VkAttachmentDescription depthAttachment;
		std::vector<VkSubpassDescription> subpasses;
		std::vector<VkSubpassDependency> dependencies;

		VkAttachmentReference depthAttachmentRef;

	public:
		RenderPassBuilder(VkDevice device);

		RenderPassBuilder& AddAttachment(VkFormat format, VkImageLayout finalLayout);
		RenderPassBuilder& AddSubpass(VkSubpassDescription subpass);
		RenderPassBuilder& AddDependency(VkSubpassDependency dependency);
		RenderPassBuilder& SetDepthAttachment(VkFormat format);

		VkRenderPass Build();
	};
}

#endif // !RENDER_PASS_BUILDER_H