#include "PipelineBuilder.h"
#include "shaderLoader.h"
using namespace wl::backend;
using namespace wl::resourcemanagement;
PipelineBuilder::PipelineBuilder(VkDevice device, VkPipelineLayout& pipelineLayout, VkRenderPass& renderPass)
{
	this->device = device;

	auto vertexShaderCode = ShaderLoader::readFile("shaders/vert.spv");
	auto fragShaderCode = ShaderLoader::readFile("shaders/frag.spv");
	vertexShaderModule = ShaderLoader::createShaderModule(vertexShaderCode, device);
	fragShaderModule = ShaderLoader::createShaderModule(fragShaderCode, device);

	vertexInputBindingDescription = ElVertexPNT::getBindingDescription();
	attributeDescriptions = ElVertexPNT::getAttributeDescriptions();

	blendMode = BlendMode::None;
	cullMode = CullMode::Back;

	this->pipelineLayout = pipelineLayout;
	this->renderPass = renderPass;

	depthStencilStateCI = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};
}

PipelineBuilder& PipelineBuilder::SetVertexShasderModule(VkShaderModule vertexShaderModule)
{
	this->vertexShaderModule = vertexShaderModule;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetFragShaderModule(VkShaderModule fragShaderModule)
{
	this->fragShaderModule = fragShaderModule;
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableVertexInput(bool value)
{
	this->isVertexInputAble = value;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetVertexInputBindingDescription(VkVertexInputBindingDescription bindDescription)
{
	this->vertexInputBindingDescription = bindDescription;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription> attributeDescriptions)
{
	this->attributeDescriptions = attributeDescriptions;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetColorAttachmentCount(int count)
{
	attachmentCount = count;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetBlendMode(BlendMode blendMode)
{
	this->blendMode = blendMode;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetCullMode(CullMode cullMode)
{
	this->cullMode = cullMode;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetDepthTest(bool enable)
{
	depthStencilStateCI.depthTestEnable = enable ? VK_TRUE : VK_FALSE;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetDepthWrite(bool enable)
{
	depthStencilStateCI.depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetDepthBoundsTest(bool enable)
{
	depthStencilStateCI.depthBoundsTestEnable = enable ? VK_TRUE : VK_FALSE;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetStencilTest(bool enable)
{
	depthStencilStateCI.stencilTestEnable = enable ? VK_TRUE : VK_FALSE;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetDepthCompareOp(VkCompareOp compareOp)
{
	depthStencilStateCI.depthCompareOp = compareOp;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetPipelineLayout(VkPipelineLayout& pipelineLayout)
{
	this->pipelineLayout = pipelineLayout;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetRenderPass(VkRenderPass& renderPass)
{
	this->renderPass = renderPass;
	return *this;
}

PipelineBuilder& wl::backend::PipelineBuilder::SetTargetSubpass(int index)
{
	targetSubpassIndex = index;
	return *this;
}

VkPipeline PipelineBuilder::Build()
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertexShaderModule,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//vertexInputInfo顶点输入
	VkPipelineVertexInputStateCreateInfo vertexInputState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBindingDescription,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
		.pVertexAttributeDescriptions = attributeDescriptions.data(),
	};
	if (!isVertexInputAble)
	{
		vertexInputState.vertexBindingDescriptionCount = 0;
		vertexInputState.pVertexAttributeDescriptions = nullptr;
		vertexInputState.vertexAttributeDescriptionCount = 0;
		vertexInputState.pVertexAttributeDescriptions = nullptr;
	}

	//TODO:
	//可选topology
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

	////depth
	//VkPipelineDepthStencilStateCreateInfo depthStencilState{
	//.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	//.depthTestEnable = VK_TRUE,
	//.depthWriteEnable = VK_TRUE,
	//.depthCompareOp = VK_COMPARE_OP_LESS,
	//.depthBoundsTestEnable = VK_FALSE,
	//.stencilTestEnable = VK_FALSE,
	//};

	//TODO:
	//是否使用动态渲染启用

	unsigned int mask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	//TODO:
	//颜色混合模式配置
	//VkPipelineColorBlendAttachmentState blendAttachment;

	/*if (blendMode == BlendMode::None)
	{
		blendAttachment = {
		.blendEnable = VK_FALSE,
		.colorWriteMask = mask
		};
	}
	else if (blendMode == BlendMode::Transparent)
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
	}*/

	//TODO:
	std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(attachmentCount, {
			.blendEnable = VK_FALSE,
			.colorWriteMask = mask
		});
	VkPipelineColorBlendStateCreateInfo colorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = attachmentCount,
		.pAttachments = blendAttachments.data()
	};

	//光栅化
	VkPipelineRasterizationStateCreateInfo rasterizationState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = static_cast<VkCullModeFlags>(cullMode),
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
	.pDepthStencilState = &depthStencilStateCI,
	.pColorBlendState = &colorBlendState,
	.pDynamicState = &dynamicState,
	.layout = pipelineLayout,
	.renderPass = renderPass,
	.subpass = static_cast<uint32_t>(targetSubpassIndex)
	};
	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline!");
	}
	return pipeline;
}
