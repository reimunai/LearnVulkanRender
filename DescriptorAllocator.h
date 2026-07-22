#pragma once
#ifndef DESCRIPTOR_ALLOCATIOR_H
#define DESCRIPTOR_ALLOCATIOR_H

#include <vector>

#include "vulkan/vulkan.h"
#include <stdexcept>

namespace wl::backend
{
	class DescriptorAllocator
	{
	public:
		VkDevice device;

		VkDescriptorPool GlobalPool;
		VkDescriptorPool ObjectUniformPool;
		VkDescriptorPool ResourcesUniformPool;

		std::vector<VkDescriptorSetLayoutBinding> globalBinds;
		std::vector<VkDescriptorSetLayoutBinding> ObjectUniformBinds;
		std::vector<VkDescriptorSetLayoutBinding> ResourcesUniformBinds;

		VkDescriptorSetLayout globalSetLayout;
		VkDescriptorSetLayout ObjectUniformLayout;
		VkDescriptorSetLayout ResourcesUniformLayout;


		enum class SETTYPE
		{
			GLOBAL,
			OBJECT_UNIFORM,
			RESOURCES_UNIFORM,
			OTHER
		};
	private:
		uint32_t flighyFrameCount = 0;
		uint32_t maxObjCount = 100;
		uint32_t maxObjUniformCount = 2;
		uint32_t maxResourceUniformCount = 100;
	public:
		DescriptorAllocator(VkDevice device, int maxFlightFrame);
		~DescriptorAllocator();
		std::vector<VkDescriptorSet> AllocateDescriptorSets(SETTYPE type);
		VkDescriptorSetLayout CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings);
		VkPipelineLayout CreatePipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
	private:
		void InitSetLayout();
		std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool pool, int setCount);
		void InitGlobalPool(uint32_t flighyFrameCount);
		void InitObjectUniformPool(uint32_t flighyFrameCount);
		void InitResourcesUniformPool();
	};
}

#endif // !DESCRIPTOR_ALLOCATIOR_H