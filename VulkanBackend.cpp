#include "VulkanBackend.h"

using namespace el::backend;
using el::VertexPC;
using el::VertexPN;
using std::vector;

const std::vector<VertexPC> vertices = {
	{{0.0f, -0.2f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.2f, 0.2f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{-0.2f, 0.2f, 0.0f}, {1.0f, 1.0f, 1.0f}},

	{{0.0f, 0.1f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.2f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{-0.2f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
};

const char* validationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};

static inline void checkVKresult(VkResult result)
{
	if (result != VK_SUCCESS)
	{
		std::cerr << "Vulkan call returned an error(" << result << std::endl;
		exit(result);
	}
}

#pragma region VulkanBackend
void VulkanBackend::Init()
{
	initSDL();
	creatInstance();
	seletePhysicsDevice();
	setupDevice();
	InitVmaAllocator();

	createSwapchain();

	SetDepthFormat();
	CreateDepthImage();
	createPresentRenderPass();
	createSwapchainFramebuffers();
	//createUniformBuffers();

	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
}
void VulkanBackend::Cleanup()
{
	//TODO

	//SyncObjects

	//commandPool
	vkDestroyCommandPool(device, commandPool, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanBackend::initSDL()
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "failed to init SDL" << std::endl;
		return;
	}

	window = SDL_CreateWindow(
		"Vulkan window",
		800,
		600,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
	);
}
void VulkanBackend::creatInstance()
{
	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "vulkan",
		.apiVersion = VK_API_VERSION_1_3
	};

	uint32_t instanceExtensionsCount = 0;
	char const* const* instanceExtensions{ SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount) };

	VkInstanceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = validationLayers,
		.enabledExtensionCount = instanceExtensionsCount,
		.ppEnabledExtensionNames = instanceExtensions,
	};
	checkVKresult(vkCreateInstance(&createInfo, nullptr, &instance));

	SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);
}
void VulkanBackend::seletePhysicsDevice()
{
	uint32_t deviceCount = 0;
	checkVKresult(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	vector<VkPhysicalDevice> devices(deviceCount);
	checkVKresult(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

	for (const auto& device : devices)
	{
		VkPhysicalDeviceProperties2 properties{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
		};
		vkGetPhysicalDeviceProperties2(device, &properties);
		
		if (properties.properties.apiVersion >= VK_API_VERSION_1_3)
		{
			if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				phyDevice = device;
			}
		}
	}

	if (phyDevice == VK_NULL_HANDLE)
	{
		std::cerr << "no suitable physics device!" << std::endl;
	}
}
void VulkanBackend::setupDevice()
{
	
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, nullptr);

	vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t queueFamily = 0;
	for (size_t i = 0; i < queueFamilies.size(); i++)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queueFamily = i;
			break;
		}
	}
	if (!SDL_Vulkan_GetPresentationSupport(instance, phyDevice, queueFamily))
	{
		std::cerr << "Get Presentation Support error!" << std::endl;
		exit(0);
	}

	
	VkDeviceQueueCreateInfo queueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities
	};

	VkPhysicalDeviceFeatures phyDeviceFeatures{};

	VkDeviceCreateInfo deviceCI{
	.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	.pNext = &enabledVk13Features,
	.queueCreateInfoCount = 1,
	.pQueueCreateInfos = &queueCreateInfo,
	.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
	.ppEnabledExtensionNames = deviceExtensions.data(),
	.pEnabledFeatures = &enabledVk10Features
	};
	checkVKresult(vkCreateDevice(phyDevice, &deviceCI, nullptr, &device));
	queueFamilyIndex = queueFamily;
	vkGetDeviceQueue(device, queueFamily, 0, &queue);
}
void VulkanBackend::InitVmaAllocator()
{
	//VmaVulkanFunctions functions{
	//	.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
	//	.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
	//	.vkCreateImage = vkCreateImage,
	//};

	VmaAllocatorCreateInfo vmaCI{};
	vmaCI.instance = instance;
	vmaCI.physicalDevice = phyDevice;
	vmaCI.device = device;

	if (vmaCreateAllocator(&vmaCI, &allocator) != VK_SUCCESS)
	{
		std::cerr << "Get Presentation Support error!" << std::endl;
	}
}


void VulkanBackend::createSwapchain()
{
	checkVKresult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevice, surface, &surfaceCaps));

	swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

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
	checkVKresult(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));

	//get swapchain VKimge
	uint32_t imageCount{ 0 };
	checkVKresult(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
	swapchainImages.resize(imageCount);
	checkVKresult(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));

	//image view
	swapChainImageViews.resize(imageCount);
	for (size_t i = 0; i < swapchainImages.size(); i++) {
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

		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}
void VulkanBackend::cleanupSwapChain()
{
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);
}
void VulkanBackend::recreateSwapchain()
{
	vkDeviceWaitIdle(device);

	cleanupSwapChain();
	createSwapchain();
	createSwapchainFramebuffers();
}

void VulkanBackend::createPresentRenderPass()
{
	//TODO:Depth test
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

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &presentRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanBackend::createSwapchainFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			swapChainImageViews[i],
			depthImageView,
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = presentRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = surfaceCaps.currentExtent.width;
		framebufferInfo.height = surfaceCaps.currentExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanBackend::SetDepthFormat()
{
	std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	for (VkFormat& format : depthFormatList) {
		VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
		vkGetPhysicalDeviceFormatProperties2(phyDevice, format, &formatProperties);
		//vkGetPhysicalDeviceFormatProperties(phyDevice, )
		if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depthFormat = format;
			break;
		}
	}
}

void VulkanBackend::CreateDepthImage()
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

	//TODO:职责混乱了
	if (vmaCreateImage(allocator, &depthImageInfo, &allocationCreateInfo, &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS)
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
	vkCreateImageView(device, &depthImageViewInfo, nullptr, &depthImageView);
}

void VulkanBackend::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueFamilyIndex,
	};

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void VulkanBackend::createCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);


	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t)commandBuffers.size()
	};

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

VkCommandBuffer VulkanBackend::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanBackend::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanBackend::RecordCommandBuffer(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout, vector<uint16_t>& indexCounts,
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

	std::array <VkClearValue , 2 > clearValues;
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
void VulkanBackend::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
			) 
		{
			throw std::runtime_error("failed to create semaphores!");
		}
	}

}

void VulkanBackend::Render(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout, vector<uint16_t>& indexCounts,
	vector<VkBuffer>& vertexBuffers, vector<VkBuffer>& indexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets)
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);


	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		recreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(device, 1, &inFlightFences[currentFrame]);


	//TODO:
	//更好的cmd提交
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	RecordCommandBuffer(renderPass, pipeline, pipelineLayout, indexCounts, vertexBuffers, indexBuffers, descriptorSets);

	//submit info
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
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
	if (vkQueueSubmit(queue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
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

	vkQueuePresentKHR(queue, &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VulkanBackend::~VulkanBackend()
{
	SDL_DestroyWindow(window);
}
#pragma endregion

#pragma region VulkanMemory
VulkanMemory::VulkanMemory(VulkanBackend& backend)
{
	this->backend = &backend;
	InitVmaAllocator();
	InitDescriptorPool();
	InitTransferCmdBuffer();
}

uint32_t VulkanMemory::GetMemoryType(uint32_t memTpyeBit, VkMemoryPropertyFlags propertyFlags)
{
	VkPhysicalDeviceMemoryProperties memPropts;
	vkGetPhysicalDeviceMemoryProperties(this->backend->phyDevice, &memPropts);
	for (uint32_t i = 0; i < memPropts.memoryTypeCount; i++)
	{
		if (((1 << i) & memTpyeBit) && (memPropts.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to Get memory Type!");
}

VkBuffer VulkanMemory::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
{
	VkBufferCreateInfo bufferCI
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkBuffer buffer;
	if (vkCreateBuffer(this->backend->device, &bufferCI, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create buffer!");
	}

	
	return buffer;
}

VkDeviceMemory VulkanMemory::AllocateDeviceMemory(VkDeviceSize allocateSize, uint32_t memoryTypeIndex)
{
	VkMemoryAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = allocateSize,
		.memoryTypeIndex = memoryTypeIndex,
	};
	VkDeviceMemory memory;
	if (vkAllocateMemory(backend->device, &allocateInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate Device memory!");
	}
	return memory;
}
void VulkanMemory::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	vkBeginCommandBuffer(transferCmdBuffer, &beginInfo);

	VkBufferCopy copy{
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};
	vkCmdCopyBuffer(transferCmdBuffer, srcBuffer, dstBuffer, 1, &copy);
	vkEndCommandBuffer(transferCmdBuffer);

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &transferCmdBuffer,
	};
	vkQueueSubmit(backend->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(backend->queue);
}
void VulkanMemory::InitDescriptorPool()
{
	/*VkDescriptorPoolSize poolSize{
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(backend->MAX_FRAMES_IN_FLIGHT)
	};

	VkDescriptorPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = static_cast<uint32_t>(backend->MAX_FRAMES_IN_FLIGHT),
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize,
	};*/

	//ImageTextureTest
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(backend->MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(backend->MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	//TODO::池过小, maxSets
	poolInfo.maxSets = static_cast<uint32_t>(backend->MAX_FRAMES_IN_FLIGHT) * 2.0f;

	if (vkCreateDescriptorPool(backend->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VulkanMemory::InitTransferCmdBuffer()
{
	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = backend->commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	vkAllocateCommandBuffers(backend->device, &allocateInfo, &transferCmdBuffer);
}

void VulkanMemory::InitVmaAllocator()
{
	allocator = backend->allocator;
}

void VulkanMemory::Cleanup()
{
	vkDestroyDescriptorPool(backend->device, descriptorPool, nullptr);
	vkFreeCommandBuffers(backend->device, backend->commandPool, 1, &transferCmdBuffer);
}

void VulkanMemory::CreateIndexBuffer(el::VertexIndice& indices, VkBuffer& buffer, VmaAllocation allocation, VmaAllocationInfo& allocationInfo)
{
	VkDeviceSize size = sizeof(indices[0]) * indices.size();
	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	};

	VmaAllocationCreateInfo allocationCreateInfo{
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};
	vmaCreateBuffer(allocator, &bufferInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo);

	void* pData;
	vmaMapMemory(allocator, allocation, &pData);
	memcpy(pData, indices.data(), size);
	vmaFlushAllocation(allocator, allocation, 0, size);
	vmaUnmapMemory(allocator, allocation);
}
void VulkanMemory::CreateVertexBuffer(vector<ElVertexPNT>& vertices, VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo& allocationInfo)
{
	VkDeviceSize size = sizeof(vertices[0]) * vertices.size();
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	};
	VmaAllocationCreateInfo allocationCreateInfo{
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};
	vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo);

	void* pData;
	vmaMapMemory(allocator, allocation, &pData);
	memcpy(pData, vertices.data(), size);
	vmaFlushAllocation(allocator, allocation, 0, size);
	vmaUnmapMemory(allocator, allocation);
	
}
void VulkanMemory::CreateUniformBuffer(VkDeviceSize bufferSize, vector<VkBuffer>& buffers, vector<VkDeviceMemory>& memorys, vector<void*>& maps)
{
	
	auto flightFrameCount = backend->MAX_FRAMES_IN_FLIGHT;
	buffers.resize(flightFrameCount);
	memorys.resize(flightFrameCount);
	maps.resize(flightFrameCount);

	for (unsigned int i = 0; i < flightFrameCount; i++)
	{
		buffers[i] = CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(backend->device, buffers[i], &memoryRequirements);
		auto propertyFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		
		memorys[i] = AllocateDeviceMemory(memoryRequirements.size, GetMemoryType(memoryRequirements.memoryTypeBits, propertyFlag));
		vkBindBufferMemory(backend->device, buffers[i], memorys[i], 0);

		vkMapMemory(backend->device, memorys[i], 0, bufferSize, 0, &maps[i]);
	}
}
vector<VkDescriptorSet> VulkanMemory::CreateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, vector<VkBuffer>& uniformBuffers)
{
	auto flightFrameCount = backend->MAX_FRAMES_IN_FLIGHT;
	std::vector<VkDescriptorSetLayout> layouts(flightFrameCount, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(flightFrameCount),
		.pSetLayouts = layouts.data()
	};

	vector<VkDescriptorSet> descriptorSets(flightFrameCount);
	if (vkAllocateDescriptorSets(backend->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < flightFrameCount; i++)
	{
		VkDescriptorBufferInfo bufferInfo{
			.buffer = uniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};

		//imageTextureTest


		VkWriteDescriptorSet descriptorWrite{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			//.pImageInfo = nullptr, // Optional
			.pBufferInfo = &bufferInfo,
			//.pTexelBufferView = nullptr // Optional
		};

		vkUpdateDescriptorSets(backend->device, 1, &descriptorWrite, 0, nullptr);
	}

	return descriptorSets;
}
#pragma endregion

#pragma region VulkanPipeline
VulkanPipeline::VulkanPipeline(VulkanBackend& backend)
{
	this->backend = &backend;
}

VkDescriptorSetLayout VulkanPipeline::CreateDescriptorSetLayout(vector<VkDescriptorSetLayoutBinding>& bindings) 
{
	VkDescriptorSetLayoutCreateInfo dscriptorSetLayoutCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VkDescriptorSetLayout layout;
	if (vkCreateDescriptorSetLayout(backend->device, &dscriptorSetLayoutCI, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	return layout;
}
VkPipelineLayout VulkanPipeline::CreatePipelineLayout(VkDescriptorSetLayout& descriptorSetLayout)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout
		//pushConstant args TODO
	};

	VkPipelineLayout pipelineLayout;
	if (vkCreatePipelineLayout(backend->device, &pipelineLayoutCI, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
	return pipelineLayout;
}
//only one render attachment.
//TODO:
//support multiply attachment render
VkRenderPass VulkanPipeline::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{
		.format = backend->swapchainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		/*	.pDepthStencilAttachment = &depthAttachmentRef,
			.pResolveAttachments = &colorAttachmentResolveRef*/
	};

	VkSubpassDependency dependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	VkRenderPass renderPass;
	if (vkCreateRenderPass(backend->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

	return renderPass;
}

VkPipeline VulkanPipeline::CreatePipeline(PipelineConfig config, VkPipelineLayout& pipelineLayout, VkRenderPass& renderPass)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = config.vertexShaderModule,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = config.fragShaderModule,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//vertexInputInfo顶点输入
	//TODO:
	//支持不同顶点数据结构
	auto bindingDescription = ElVertexPNT::getBindingDescription();
	auto attributeDescriptions = ElVertexPNT::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
		.pVertexAttributeDescriptions = attributeDescriptions.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	//视口
	VkPipelineViewportStateCreateInfo viewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};

	std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStates.data()
	};

	//depth
	VkPipelineDepthStencilStateCreateInfo depthStencilState{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	.depthTestEnable = VK_TRUE,
	.depthWriteEnable = VK_TRUE,
	.depthCompareOp = VK_COMPARE_OP_LESS,
	.depthBoundsTestEnable = VK_FALSE,
	.stencilTestEnable = VK_FALSE,
	};

	//TODO:
	//是否使用动态渲染启用

	unsigned int mask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	//TODO:
	//颜色混合模式配置
	VkPipelineColorBlendAttachmentState blendAttachment;

	if (config.blendMode == BlendMode::None)
	{
		blendAttachment = {
		.blendEnable = VK_FALSE,
		.colorWriteMask = mask
		};
	}
	else if (config.blendMode == BlendMode::Transparent)
	{
		blendAttachment = {
		.blendEnable = VK_TRUE,

		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,

		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,

		.colorWriteMask = mask,
		};
	}

	VkPipelineColorBlendStateCreateInfo colorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &blendAttachment
	};

	//光栅化
	VkPipelineRasterizationStateCreateInfo rasterizationState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = static_cast<VkCullModeFlags>(config.cullMode),
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
	};

	//TODO:
	//多重采样设置
	VkPipelineMultisampleStateCreateInfo multisampleState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
	};

	VkGraphicsPipelineCreateInfo pipelineCI{
	.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	.stageCount = 2,
	.pStages = shaderStages,
	.pVertexInputState = &vertexInputState,
	.pInputAssemblyState = &inputAssembly,
	.pViewportState = &viewportState,
	.pRasterizationState = &rasterizationState,
	.pMultisampleState = &multisampleState,
	.pDepthStencilState = &depthStencilState,
	.pColorBlendState = &colorBlendState,
	.pDynamicState = &dynamicState,
	.layout = pipelineLayout,
	.renderPass = renderPass,
	.subpass = 0
	};
	VkPipeline pipeline;
	if(vkCreateGraphicsPipelines(backend->device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline!");
	}
	return pipeline;
}
#pragma endregion