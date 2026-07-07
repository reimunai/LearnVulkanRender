#include "PbrTest.h"

#include "VkBackend.h"
#include "VkRender.h"
#include "VkResources.h"
#include "shaderLoader.h"

#include "Buffer.h"
#include "PipelineBuilder.h"
#include "RenderPassBuilder.h"
#include "GltfLoader.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <iostream>
#include <vector>

using namespace wl::backend;
using wl::resourcemanagement::ShaderLoader;
using std::vector;
using wl::backend::PipelineBuilder;
using wl::backend::RenderPassBuilder;
using wl::v1::backend::Buffer;

namespace test
{ 
	SDL_Window* window = nullptr;
	VkBackend* backend = nullptr;
	VkResources* vkres = nullptr;
	VkRenderPresent* vkRenderPresent = nullptr;

	VkRenderPass renderPass;
	VkPipeline gbufferPipeline;

	const string meshFilePath = "meshs/cube.gltf";
	static bool InitWindowAndVulkan()
	{
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			std::cerr << "failed to init SDL" << std::endl;
			return false;
		}

		window = SDL_CreateWindow("Vulkan", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

		backend = new VkBackend();
		vkres = new VkResources(*backend);
		vkRenderPresent = new VkRenderPresent(*backend, vkres->vmaAllocator, window);
		return true;
	}

	static void LoadSahderCode(string filePath, VkShaderModule& shaderModule)
	{
			auto ShaderCode = ShaderLoader::readFile(filePath);
			shaderModule = ShaderLoader::createShaderModule(ShaderCode, backend->device);
	}

	static void ConfigPipeline(VkPipelineLayout& pipelineLayout, VkShaderModule vertShaderModule, VkShaderModule fragShaderModule)
	{
		vector<VkAttachmentReference> colorAttachmentRefs{
			{
				//out albedo
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			},
			{
				//out normal
				.attachment = 1,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}
		};
		VkAttachmentReference depthAttachmentRef{
			.attachment = 2,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		renderPass = RenderPassBuilder(backend->device)
			//out albedo
			.AddAttachment(vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			//out normal
			.AddAttachment(vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.SetDepthAttachment(vkRenderPresent->depthFormat)
			.AddSubpass(
				{
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size()),
					.pColorAttachments = colorAttachmentRefs.data(),
					.pDepthStencilAttachment = &depthAttachmentRef
				}
			)
			.AddDependency(
				{
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				}
				)
			.Build();

		gbufferPipeline =
			PipelineBuilder(backend->device, pipelineLayout, renderPass)
			.SetVertexShasderModule(vertShaderModule)
			.SetFragShaderModule(fragShaderModule)
			.SetBindingDescription(wl::ElVertexPNT::getBindingDescription())
			.SetAttributeDescriptions(wl::ElVertexPNT::getAttributeDescriptions())
			.SetBlendMode(wl::BlendMode::None)
			.SetCullMode(wl::CullMode::Back)
			.SetDepthTest(true)
			.SetDepthWrite(true)
			.SetDepthCompareOp(VK_COMPARE_OP_LESS)
			.Build();
	}
}
inline void test::Test_Run()
{
	if (!InitWindowAndVulkan())
	{
		return;
	}

	Uint64 startTime = SDL_GetPerformanceCounter();
	Uint64 frequency = SDL_GetPerformanceFrequency();

	VkShaderModule vertShaderModule, fragShaderModule;
	LoadSahderCode("shaders/vert.spv", vertShaderModule);
	LoadSahderCode("shaders/frag.spv", fragShaderModule);

	vector<VkDescriptorSetLayoutBinding> binds{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,//no array
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr  // Optional
		}
	};
	auto setlayout = vkres->CreateDescriptorSetLayout(binds);
	auto pipelineLayout = vkres->CreatePipelineLayout(setlayout);

	//config pipeline
	ConfigPipeline(pipelineLayout, vertShaderModule, fragShaderModule);

	vector<Buffer> uniformBuffers;
	auto dscrptSets = vkres->CreateDescriptorSets(setlayout);
	wl::MVPUniform ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.model = modelMat4;
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), vkRenderPresent->surfaceCaps.currentExtent.width / (float)vkRenderPresent->surfaceCaps.currentExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;
	for( int i = 0; i < 2; i++)
	{
		auto buffer =
			Buffer::Builder(vkres->vmaAllocator, sizeof(wl::MVPUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
			.Build();

		uniformBuffers.push_back(buffer);

		buffer.CopyDataToBuffer(&ubo);
	}

	//UpdateDscrpSets
	for (size_t i = 0; i < 2; i++)
	{
		VkDescriptorBufferInfo bufferInfo{
			.buffer = uniformBuffers[i].vkBuffer,
			.offset = 0,
			.range = sizeof(wl::MVPUniform)
		};

		vector<VkWriteDescriptorSet> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = dscrptSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(backend->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	vector<wl::ElVertexPNT> vertices;
	wl::VertexIndice indices;
	LoadGLTFMeshVertices(meshFilePath, vertices, indices);

	//el::ElBuffer vertexBuffer;
	//el::ElBuffer indexBuffer;

	Buffer::Builder bufferBulider(vkres->vmaAllocator, sizeof(wl::ElVertexPNT)* vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	Buffer vertexBuffer2 =
		bufferBulider
		.Build();

	vertexBuffer2.CopyDataToBuffer(vertices.data());

	Buffer indexBuffer2 =
		bufferBulider
		.SetBufferUsage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
		.Build();

	indexBuffer2.CopyDataToBuffer(indices.data());

	vector<uint16_t> indexCounts = { static_cast<uint16_t>(indices.size()) };
	vector<VkBuffer> vertexBuffers = { vertexBuffer2.vkBuffer };

	vector<VkBuffer> indexBuffers = { indexBuffer2.vkBuffer };
	vector<vector<VkDescriptorSet>> setSets{ dscrptSets };
}