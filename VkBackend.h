#pragma once
#ifndef VKBACKEND_H
#include "vulkan/vulkan.h"
#include <vector>
namespace wl
{
	namespace backend
	{
		//TODO:多线程支持
		class VkBackend
		{
		public:
			//instance and device
			VkInstance instance = nullptr;
			VkPhysicalDevice phyDevice = nullptr;
			VkDevice device = nullptr;
			//queue
			VkQueue queue = nullptr;
			uint32_t queueFamilyIndex = 0;
			//vector<queue> queues;
			//vector<uint32_t> queueFamilyIndexs;
			//cmdpool
			VkCommandPool cmdPool;
		private:
			const float queuePriorities = 1.0f;
			std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

			const std::vector<const char*> deviceExtensions{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			VkPhysicalDeviceVulkan12Features enabledVk12Features{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
				.descriptorIndexing = true,
				.shaderSampledImageArrayNonUniformIndexing = true,
				.descriptorBindingVariableDescriptorCount = true,
				.runtimeDescriptorArray = true,
				.bufferDeviceAddress = true
			};
			const VkPhysicalDeviceVulkan13Features enabledVk13Features{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
				.pNext = &enabledVk12Features,
				.synchronization2 = true,
				.dynamicRendering = true,
			};
			const VkPhysicalDeviceFeatures enabledVk10Features{
				.samplerAnisotropy = VK_TRUE
			};
		public:
			VkBackend();
			~VkBackend();

			void Init();
			void Cleanup();

			VkCommandBuffer BeginOneTimesCommands();
			void EndOneTimesCommands(VkCommandBuffer commandBuffer);

		private:
			void CreatInstance();
			void SetPhysicsDevice();
			void CreateDevice();

			void CreateCmdPool();
		};
	}
}
#endif // !VKBACKEND_H

