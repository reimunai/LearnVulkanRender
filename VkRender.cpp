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
	CreateSurface(window);
	CreateSwapchain();


	GetSuitalbeDepthFormat();

	CreateCommandPool();
	CreateCommandBuffers();
	CreateSyncObjects();
}

void VkRenderPresent::CreateSurface(SDL_Window* window)
{
	SDL_Vulkan_CreateSurface(window, vkBackend->instance, nullptr, &surface);
}

VkFormat VkRenderPresent::GetSuitalbeDepthFormat()
{
	VkFormat res;
	std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	for (VkFormat& format : depthFormatList) {
		VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
		vkGetPhysicalDeviceFormatProperties2(vkBackend->phyDevice, format, &formatProperties);
		//vkGetPhysicalDeviceFormatProperties(phyDevice, )
		if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			res = format;
			break;
		}
	}

	return res;
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

void VkRenderPresent::RecreateSwapchain()
{
	vkDeviceWaitIdle(vkBackend->device);

	CleanupSwapChain();
	CreateSwapchain();
}

//TODO:
void VkRenderPresent::CleanupSwapChain()
{
	/*for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(vkBackend->device, framebuffer, nullptr);
	}

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(vkBackend->device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(vkBackend->device, swapchain, nullptr);*/
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

void VkRenderPresent::InitRecordCmdBuffer()
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
}

void VkRenderPresent::BeginRenderPassCmd(VkRenderPass& renderPass, std::vector<VkClearValue>& clearValues, std::vector<VkFramebuffer>& frameBuffers)
{
	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = frameBuffers[currentFrame],
		.renderArea{.offset = {0, 0},	.extent = surfaceCaps.currentExtent},
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues = clearValues.data()
	};
	vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VkRenderPresent::BindPipelineCmd(VkPipeline& pipeline)
{
	vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void VkRenderPresent::BindDataCmd(VkBuffer vertexBuffer, VkBuffer indexBuffer, VkPipelineLayout& pipelineLayout, std::vector<VkDescriptorSet>& dscrpsets)
{
	std::array<VkDeviceSize, 1> offsets{0};
	vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, &vertexBuffer, offsets.data());
	vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(
		commandBuffers[currentFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&dscrpsets[currentFrame],
		0,
		nullptr
	);
}

void wl::backend::VkRenderPresent::BindVertexCmd(VkBuffer vertexBuffer)
{
	std::array<VkDeviceSize, 1> offsets{ 0 };
	vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, &vertexBuffer, offsets.data());
}

void wl::backend::VkRenderPresent::BindIndexCmd(VkBuffer indexBuffer)
{
	vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void wl::backend::VkRenderPresent::BindDescriptorSets(VkPipelineLayout& pipelineLayout, std::vector<VkDescriptorSet>& dscrpsets)
{
	vkCmdBindDescriptorSets(
		commandBuffers[currentFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&dscrpsets[currentFrame],
		0,
		nullptr
	);
}

void VkRenderPresent::IndexDrawCmd(int indexCount)
{
	vkCmdDrawIndexed(commandBuffers[currentFrame], static_cast<uint32_t>(indexCount), 1, 0, 0, 0);
}

void VkRenderPresent::DrawCmd(int count)
{
	vkCmdDraw(commandBuffers[currentFrame], static_cast<uint32_t>(count), 1, 0, 0);
}

void VkRenderPresent::NextSubPass()
{
	vkCmdNextSubpass(commandBuffers[currentFrame], VK_SUBPASS_CONTENTS_INLINE);
}

void VkRenderPresent::EndRenderPassCmd()
{
	vkCmdEndRenderPass(commandBuffers[currentFrame]);
}

void VkRenderPresent::EndRecordCmdBuffer()
{
	if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VkRenderPresent::ResetCmd()
{
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
}

void VkRenderPresent::WaitReady()
{
	vkWaitForFences(vkBackend->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(vkBackend->device, 1, &inFlightFences[currentFrame]);
}

void VkRenderPresent::Render()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vkBackend->device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	/*if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}*/

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