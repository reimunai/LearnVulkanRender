#include "VkRender.h"

#include <iostream>
#include <array>

#include "FrameBufferBuilder.h"

using namespace wl::backend;
using std::vector;

VkRenderPresent::VkRenderPresent(VkBackend& backend, VmaAllocator vmaAllocator, SDL_Window* window)
{
	this->vmaAllocator = vmaAllocator;
	this->vkBackend = &backend;
	//InitSDL("Vulkan", 800, 600);
	CreateSurface(window);
	CreateSwapchain();


	SetDepthFormat();
	CreateDepthImage();
	CreatePresentRenderPass();
	CreateSwapchainFramebuffers();

	CreateCommandPool();
	CreateCommandBuffers();
	CreateSyncObjects();
}
//void VkRenderPresent::InitSDL(const char* title, int w, int h)
//{
//	if (!SDL_Init(SDL_INIT_VIDEO))
//	{
//		std::cerr << "failed to init SDL" << std::endl;
//		return;
//	}
//
//	window = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
//}

void VkRenderPresent::CreateSurface(SDL_Window* window)
{
	SDL_Vulkan_CreateSurface(window, vkBackend->instance, nullptr, &surface);
}

void VkRenderPresent::SetDepthFormat()
{
	std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	for (VkFormat& format : depthFormatList) {
		VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
		vkGetPhysicalDeviceFormatProperties2(vkBackend->phyDevice, format, &formatProperties);
		//vkGetPhysicalDeviceFormatProperties(phyDevice, )
		if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depthFormat = format;
			break;
		}
	}
}

void VkRenderPresent::CreateDepthImage()
{
	VkImageCreateInfo depthImageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = depthFormat,
		.extent{
			.width = surfaceCaps.currentExtent.width,
			.height = surfaceCaps.currentExtent.height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo allocationCreateInfo{
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};

	if (vmaCreateImage(vmaAllocator, &depthImageInfo, &allocationCreateInfo, &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Image!");
	}

	VkImageViewCreateInfo depthImageViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = depthImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = depthFormat,
		.subresourceRange{
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.layerCount = 1
		},
	};
	vkCreateImageView(vkBackend->device, &depthImageViewInfo, nullptr, &depthImageView);
}

void VkRenderPresent::CreateSwapchain()
{
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkBackend->phyDevice, surface, &surfaceCaps) != VK_SUCCESS)
	{
		std::cerr << "failed get surface capability!" << std::endl;
	}

	//TODO:
	//确保正确的交互链图像格式转换
	VkSwapchainCreateInfoKHR swapchainCI{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = surfaceCaps.minImageCount,
		.imageFormat = swapchainImageFormat,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.imageExtent{.width = surfaceCaps.currentExtent.width, .height = surfaceCaps.currentExtent.height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR
	};

	VkResult result = vkCreateSwapchainKHR(vkBackend->device, &swapchainCI, nullptr, &swapchain);
	if (result != VK_SUCCESS)
	{
		std::cerr << "failed to create swapchain! " << result << std::endl;
	}

	//get swapchain VKimge
	uint32_t imageCount{ 0 };
	vkGetSwapchainImagesKHR(vkBackend->device, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	if (vkGetSwapchainImagesKHR(vkBackend->device, swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS)
	{
		std::cerr << "failed to get swapchain images!" << std::endl;
	}

	//image view
	swapChainImageViews.resize(imageCount);
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(vkBackend->device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void VkRenderPresent::CreateSwapchainFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	FrameBufferBuilder frameBufferBuilder{
		vkBackend->device,
		presentRenderPass,
		surfaceCaps.currentExtent.width,
		surfaceCaps.currentExtent.height
	};

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::vector<VkImageView> attachments = {
			swapChainImageViews[i],
			depthImageView,
		};

		swapChainFramebuffers[i] =
			frameBufferBuilder
			.SetAttackments(attachments)
			.Build();
	}
}

void VkRenderPresent::RecreateSwapchain()
{
	vkDeviceWaitIdle(vkBackend->device);

	CleanupSwapChain();
	CreateSwapchain();
	CreateDepthImage();
	CreateSwapchainFramebuffers();
}

void VkRenderPresent::CleanupSwapChain()
{
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(vkBackend->device, framebuffer, nullptr);
	}

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(vkBackend->device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(vkBackend->device, swapchain, nullptr);
}

void VkRenderPresent::CreatePresentRenderPass()
{
	VkAttachmentDescription colorAttachment{
		.format = swapchainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkAttachmentDescription depthAttachment{
		.format = depthFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkAttachmentReference depthAttachmentRef{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef,
		/*.pResolveAttachments = &colorAttachmentResolveRef*/
	};

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	if (vkCreateRenderPass(vkBackend->device, &renderPassInfo, nullptr, &presentRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void VkRenderPresent::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vkBackend->queueFamilyIndex,
	};

	if (vkCreateCommandPool(vkBackend->device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void VkRenderPresent::CreateCommandBuffers()
{
	commandBuffers.resize(maxFramesInFlight);


	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t)commandBuffers.size()
	};

	if (vkAllocateCommandBuffers(vkBackend->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void VkRenderPresent::CreateSyncObjects()
{
	imageAvailableSemaphores.resize(maxFramesInFlight);
	renderFinishedSemaphores.resize(maxFramesInFlight);
	inFlightFences.resize(maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(vkBackend->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(vkBackend->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(vkBackend->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
			)
		{
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

//TODO:更好的cmd提交
void VkRenderPresent::RecordCommandBuffer(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout, vector<uint16_t>& indexCounts,
	vector<VkBuffer>& vertexBuffers, vector<VkBuffer>& indexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets)
{
	VkCommandBuffer cmdBuffer = commandBuffers[currentFrame];
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		//.flags = 0; // Optional
		//.pInheritanceInfo = nullptr; // Optional
	};
	if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkViewport viewport{
	.x = 0.0f,
	.y = 0.0f,
	.width = static_cast<float>(surfaceCaps.currentExtent.width),
	.height = static_cast<float>(surfaceCaps.currentExtent.height),
	.minDepth = 0.0f,
	.maxDepth = 1.0f
	};
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = surfaceCaps.currentExtent
	};
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	std::array <VkClearValue, 2 > clearValues;
	clearValues[0] = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = swapChainFramebuffers[currentFrame],
		.renderArea{.offset = {0, 0},	.extent = surfaceCaps.currentExtent},
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues = clearValues.data()
	};
	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	//uint32_t count = vertexBuffers.size();
	//vector<VkDeviceSize> offsets(count, 0);
	//vkCmdBindVertexBuffers(cmdBuffer, 0, count, vertexBuffers.data(), offsets.data());
	//vkCmdBindDescriptorSets(
	//	cmdBuffer,
	//	VK_PIPELINE_BIND_POINT_GRAPHICS,
	//	pipelineLayout,
	//	0,
	//	1,
	//	&descriptorSets[currentFrame],
	//	0,
	//	nullptr);

	//vkCmdDraw(cmdBuffer, 12, 1, 0, 0);
	for (uint32_t i = 0; i < vertexBuffers.size(); i++)
	{
		uint32_t count = 1;
		vector<VkDeviceSize> offsets(count, 0);
		vkCmdBindVertexBuffers(cmdBuffer, 0, count, &vertexBuffers[i], offsets.data());
		//TODO:注意VK_INDEX_TYPE_UINT32 or VK_INDEX_TYPE_UINT16
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffers[i], 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(
			cmdBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&descriptorSets[i][currentFrame],
			0,
			nullptr);

		//vkCmdDraw(cmdBuffer, 12, 1, 0, 0);
		vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(indexCounts[i]), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(cmdBuffer);

	if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VkRenderPresent::Render(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout, vector<uint16_t>& indexCounts,
	vector<VkBuffer>& vertexBuffers, vector<VkBuffer>& indexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets)
{
	vkWaitForFences(vkBackend->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);


	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vkBackend->device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(vkBackend->device, 1, &inFlightFences[currentFrame]);


	//TODO:
	//更好的cmd提交
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	RecordCommandBuffer(renderPass, pipeline, pipelineLayout, indexCounts, vertexBuffers, indexBuffers, descriptorSets);

	//submit info
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,
	};

	VkResult result1 = vkQueueSubmit(vkBackend->queue, 1, &submitInfo, inFlightFences[currentFrame]);
	if (result1 != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!" + result);
	}
	//present info
	VkSwapchainKHR swapChains[] = { swapchain };
	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &imageIndex,
		.pResults = nullptr,// Optional
	};

	vkQueuePresentKHR(vkBackend->queue, &presentInfo);

	currentFrame = (currentFrame + 1) % maxFramesInFlight;
}
