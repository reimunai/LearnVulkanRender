#include "VkBackend.h"
#include <SDL3/SDL_vulkan.h>
#include <iostream>
using namespace wl::backend;

VkBackend::VkBackend()
{
	Init();
}

VkBackend::~VkBackend()
{
	Cleanup();
}

void VkBackend::Init()
{
	CreatInstance();
	SetPhysicsDevice();
	CreateDevice();
	CreateCmdPool();
}

void VkBackend::Cleanup()
{
	vkDestroyCommandPool(device, cmdPool, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}

VkCommandBuffer VkBackend::BeginOneTimesCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void VkBackend::EndOneTimesCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}

void VkBackend::CreatInstance()
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
		.ppEnabledLayerNames = validationLayers.data(),
		.enabledExtensionCount = instanceExtensionsCount,
		.ppEnabledExtensionNames = instanceExtensions,
	};
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		std::cerr << "failed to create instance!" << std::endl;
	}
}

//TODO:
//物理设备功能检查
void VkBackend::SetPhysicsDevice()
{
	uint32_t deviceCount = 0;
	if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS)
	{
		std::cerr << "failed to enumerate physicalDevices!" << std::endl;
	}
	
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

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

//TODO: 
// 完善队列族检查，交换链支持检查等
void VkBackend::CreateDevice()
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
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
	if (vkCreateDevice(phyDevice, &deviceCI, nullptr, &device) != VK_SUCCESS)
	{
		std::cerr << "failed to create logical device!" << std::endl;
	}
	queueFamilyIndex = queueFamily;
	vkGetDeviceQueue(device, queueFamily, 0, &queue);
}

void VkBackend::CreateCmdPool()
{
	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueFamilyIndex,
	};

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &cmdPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}
