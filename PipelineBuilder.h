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

		bool isVertexInputAble = true;
		VkVertexInputBindingDescription vertexInputBindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		uint32_t attachmentCount = 1;
		BlendMode blendMode;

		CullMode cullMode;

		VkPipelineLayout pipelineLayout;
		VkRenderPass renderPass;
		int targetSubpassIndex = 0;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};

	public :
		PipelineBuilder(VkDevice device, VkPipelineLayout& pipelineLayout, VkRenderPass& renderPass);

		PipelineBuilder& SetVertexShasderModule(VkShaderModule vertexShaderModule);
		PipelineBuilder& SetFragShaderModule(VkShaderModule fragShaderModule);

		PipelineBuilder& EnableVertexInput(bool value);
		PipelineBuilder& SetVertexInputBindingDescription(VkVertexInputBindingDescription bindDescription);
		PipelineBuilder& SetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription> attributeDescriptions);

		PipelineBuilder& SetColorAttachmentCount(int count);

		PipelineBuilder& SetBlendMode(BlendMode blendMode);
		PipelineBuilder& SetCullMode(CullMode cullMode);

		PipelineBuilder& SetDepthTest(bool enable);
		PipelineBuilder& SetDepthWrite(bool enable);
		PipelineBuilder& SetDepthBoundsTest(bool enable);
		PipelineBuilder& SetStencilTest(bool enable);
		PipelineBuilder& SetDepthCompareOp(VkCompareOp compareOp);

		PipelineBuilder& SetPipelineLayout(VkPipelineLayout& pipelineLayout);
		PipelineBuilder& SetRenderPass(VkRenderPass& renderPass);
		PipelineBuilder& SetTargetSubpass(int index);

		VkPipeline Build();

	};

}

#endif // !PIPELINEBUILDER_H
