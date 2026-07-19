#include "RenderPassBuilder.h"
#include <stdexcept>

using namespace wl::backend;

RenderPassBuilder::RenderPassBuilder(VkDevice device)
{
	this->device = device;
}

RenderPassBuilder& RenderPassBuilder::AddAttachment(VkFormat format, VkImageLayout finalLayout)
{
	Attachments.push_back({
		.format = format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = finalLayout
		});

	return *this;
}

RenderPassBuilder& RenderPassBuilder::AddSubpass(VkSubpassDescription subpass)
{
	subpasses.push_back(subpass);
	return *this;
}

RenderPassBuilder& RenderPassBuilder::AddDependency(VkSubpassDependency dependency)
{
	dependencies.push_back(dependency);
	return *this;
}

RenderPassBuilder& RenderPassBuilder::SetDepthAttachment(VkFormat format)
{
	depthAttachment = {
		.format = format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	return *this;
}

VkRenderPass RenderPassBuilder::Build()
{
	VkRenderPass renderPass;

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(Attachments.size()),
		.pAttachments = Attachments.data(),
		.subpassCount = static_cast<uint32_t>(subpasses.size()),
		.pSubpasses = subpasses.data(),
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.size() > 0 ? dependencies.data() : nullptr
	};

	if( vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

	return renderPass;
}