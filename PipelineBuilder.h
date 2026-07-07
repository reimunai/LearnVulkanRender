#pragma once
#ifndef PIPELINEBUILDER_H
#define PIPELINEBUILDER_H

#include "vulkan/vulkan.h"
#include "VertexStruct.h"
#include <vector>

namespace wl::backend
{
	//TODO:more config
	class PipelineBuilder
	{
	public:
		VkDevice device;

		VkShaderModule vertexShaderModule;
		VkShaderModule fragShaderModule;

		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		BlendMode blendMode;

		CullMode cullMode;

		VkPipelineLayout pipelineLayout;
		VkRenderPass renderPass;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};

	public :
		PipelineBuilder(VkDevice device, VkPipelineLayout& pipelineLayout, VkRenderPass& renderPass);

		PipelineBuilder SetVertexShasderModule(VkShaderModule vertexShaderModule);
		PipelineBuilder SetFragShaderModule(VkShaderModule fragShaderModule);

		PipelineBuilder SetBindingDescription(VkVertexInputBindingDescription bindDescription);
		PipelineBuilder SetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription> attributeDescriptions);

		PipelineBuilder SetBlendMode(BlendMode blendMode);
		PipelineBuilder SetCullMode(CullMode cullMode);

		PipelineBuilder SetDepthTest(bool enable);
		PipelineBuilder SetDepthWrite(bool enable);
		PipelineBuilder SetDepthBoundsTest(bool enable);
		PipelineBuilder SetStencilTest(bool enable);
		PipelineBuilder SetDepthCompareOp(VkCompareOp compareOp);

		PipelineBuilder SetPipelineLayout(VkPipelineLayout& pipelineLayout);
		PipelineBuilder SetRenderPass(VkRenderPass& renderPass);

		VkPipeline Build();

	};

}

#endif // !PIPELINEBUILDER_H
