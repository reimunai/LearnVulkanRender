#include "DescriptorAllocator.h"
#include <vector>
using namespace wl::backend;
DescriptorAllocator::DescriptorAllocator(VkDevice device, int maxFlightFrame)
{
	flighyFrameCount = static_cast<uint32_t>(maxFlightFrame);
	this->device = device;
	InitSetLayout();

	InitGlobalPool(flighyFrameCount);

	InitObjectUniformPool(flighyFrameCount);

	InitResourcesUniformPool();
}
wl::backend::DescriptorAllocator::~DescriptorAllocator()
{
	vkDestroyDescriptorPool(device, GlobalPool, nullptr);
	vkDestroyDescriptorPool(device, ObjectUniformPool, nullptr);
	vkDestroyDescriptorPool(device, ResourcesUniformPool, nullptr);

	vkDestroyDescriptorSetLayout(device, globalSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, ObjectUniformLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, ResourcesUniformLayout, nullptr);


}
void wl::backend::DescriptorAllocator::InitSetLayout()
{
	globalBinds = {
		{//view,proj matrix
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
		}
	};

	ObjectUniformBinds = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
		}
	};

	ResourcesUniformBinds = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		}
	};

	globalSetLayout = CreateDescriptorSetLayout(globalBinds);
	ObjectUniformLayout = CreateDescriptorSetLayout(ObjectUniformBinds);
	ResourcesUniformLayout = CreateDescriptorSetLayout(ResourcesUniformBinds);
}

std::vector<VkDescriptorSet> DescriptorAllocator::AllocateDescriptorSets(SETTYPE type)
{
	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	int count = flighyFrameCount;
	switch (type) {
	case SETTYPE::GLOBAL:
		pool = GlobalPool;
		layout = globalSetLayout;
		break;
	case SETTYPE::OBJECT_UNIFORM:
		pool = ObjectUniformPool;
		layout = ObjectUniformLayout;
		break;
	case SETTYPE::RESOURCES_UNIFORM:
		pool = ResourcesUniformPool;
		layout = ResourcesUniformLayout;
		count = 1;
		break;
	default:
		pool = {};
		layout = {};
	}
	return AllocateDescriptorSets(layout, pool, count);
}

VkDescriptorSetLayout DescriptorAllocator::CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	VkDescriptorSetLayoutCreateInfo dscriptorSetLayoutCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VkDescriptorSetLayout layout;
	if (vkCreateDescriptorSetLayout(device, &dscriptorSetLayoutCI, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	return layout;
}

VkPipelineLayout DescriptorAllocator::CreatePipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
		.pSetLayouts = descriptorSetLayouts.data(),
		//pushConstant args TODO
	};

	VkPipelineLayout pipelineLayout;
	if (vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
	return pipelineLayout;
}

void wl::backend::DescriptorAllocator::InitGlobalPool(uint32_t flighyFrameCount)
{
	std::vector<VkDescriptorPoolSize> poolsizes0{
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = flighyFrameCount
		}
	};

	VkDescriptorPoolCreateInfo poolInfo0{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = flighyFrameCount,
		.poolSizeCount = static_cast<uint32_t>(poolsizes0.size()),
		.pPoolSizes = poolsizes0.data(),
	};

	if (vkCreateDescriptorPool(device, &poolInfo0, nullptr, &GlobalPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create global descriptor pool!");
	}
}

void wl::backend::DescriptorAllocator::InitResourcesUniformPool()
{
	std::vector<VkDescriptorPoolSize> poolsizes2{
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = maxResourceUniformCount
		}
	};

	VkDescriptorPoolCreateInfo poolInfo2{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = maxResourceUniformCount,
		.poolSizeCount = static_cast<uint32_t>(poolsizes2.size()),
		.pPoolSizes = poolsizes2.data(),
	};

	if (vkCreateDescriptorPool(device, &poolInfo2, nullptr, &ResourcesUniformPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create global descriptor pool!");
	}
}

void wl::backend::DescriptorAllocator::InitObjectUniformPool(uint32_t flighyFrameCount)
{
	std::vector<VkDescriptorPoolSize> poolsizes1{
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = flighyFrameCount * maxObjCount * maxObjUniformCount
		}
	};

	VkDescriptorPoolCreateInfo poolInfo1{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = flighyFrameCount * maxObjCount,
		.poolSizeCount = static_cast<uint32_t>(poolsizes1.size()),
		.pPoolSizes = poolsizes1.data(),
	};

	if (vkCreateDescriptorPool(device, &poolInfo1, nullptr, &ObjectUniformPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create object uniform descriptor pool!");
	}
}

std::vector<VkDescriptorSet> DescriptorAllocator::AllocateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool pool, int setCount)
{
	std::vector<VkDescriptorSetLayout> layouts(setCount, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = static_cast<uint32_t>(setCount),
		.pSetLayouts = layouts.data()
	};

	std::vector<VkDescriptorSet> descriptorSets(setCount);
	if (vkAllocateDescriptorSets(this->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	return descriptorSets;
}