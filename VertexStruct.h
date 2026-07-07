#ifndef VERTEXSTRUCT_H
#define VERTEXSTRUCT_H
#pragma once
#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include "vma/vk_mem_alloc.h"
namespace wl
{
	//TODO: need typedef name header file
	typedef std::vector<uint32_t> VertexIndice;
	struct ElVertexPNT
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription();

		//static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
	};
	struct MVPUniform
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	struct ElBuffer
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};

	enum class CullMode
	{
		None = VK_CULL_MODE_NONE,
		Front = VK_CULL_MODE_FRONT_BIT,
		Back = VK_CULL_MODE_BACK_BIT,
		FrondAanBack = VK_CULL_MODE_FRONT_AND_BACK,

	};
	enum class BlendMode
	{
		None,
		Transparent,
	};

	struct PipelineConfig
	{
		//shader
		//vertex input descriptor
		//inputassembly
			//topology mode
		//rasterizationState 
			//polygonMode
			//cullmode
			//front face
		//color blend
		uint32_t shaderID;
		CullMode cullMode;
		BlendMode blendMode;

		VkShaderModule vertexShaderModule;
		VkShaderModule fragShaderModule;
	};
}

#endif // !VERTEXSTRUCT_H