#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "shaderLoader.h"
#include "GltfLoader.h"
#include <string>

#include <stb_image.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "VkBackend.h"
#include "VkRender.h"
#include "VkResources.h"

#include "PipelineBuilder.h"
#include "RenderPassBuilder.h"
#include "Buffer.h"
#include "Image.h"
#include <iostream>

using namespace wl::backend;

const string meshFilePath = "meshs/cube.gltf";

int main()
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "failed to init SDL" << std::endl;
		return 0;
	}

	SDL_Window* window = SDL_CreateWindow("Vulkan", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	VkBackend* backend = new VkBackend();
	VkResources* vkres = new VkResources(*backend);
	VkRenderPresent* vkRenderPresent = new VkRenderPresent(*backend, vkres->vmaAllocator, window);
	Uint64 startTime = SDL_GetPerformanceCounter();
	Uint64 frequency = SDL_GetPerformanceFrequency();


	auto vertShaderCode = wl::resourcemanagement::ShaderLoader::readFile("shaders/vert.spv");
	auto fragShaderCode = wl::resourcemanagement::ShaderLoader::readFile("shaders/frag.spv");
	VkShaderModule vertShaderModule = wl::resourcemanagement::ShaderLoader::createShaderModule(vertShaderCode, backend->device);
	VkShaderModule fragShaderModule = wl::resourcemanagement::ShaderLoader::createShaderModule(fragShaderCode, backend->device);

	vector<VkDescriptorSetLayoutBinding> binds = {

		{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		//.pImmutableSamplers = nullptr  Optional
		},

		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			//.pImmutableSamplers = nullptr  Optional
		}
	};

	VkDescriptorSetLayout setlayout = vkres->CreateDescriptorSetLayout(binds);
	VkPipelineLayout pipelineLayout = vkres->CreatePipelineLayout(setlayout);
	//VkRenderPass renderPass = vkRenderPresent->presentRenderPass;
	RenderPassBuilder renderPassBuilder(backend->device);

	VkAttachmentReference colorAttachmentRef{
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkAttachmentReference depthAttachmentRef{
		1,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	VkRenderPass renderPass = 
		renderPassBuilder
		.AddAttachment(vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		.AddAttachment(vkRenderPresent->depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		.AddSubpass(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachmentRef,
				.pDepthStencilAttachment = &depthAttachmentRef
			}
		)
		.AddDependency(
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
			}
		)
		.Build();

	PipelineBuilder pipelineBuilder(backend->device, pipelineLayout, renderPass);
	VkPipeline pipeline = 
		pipelineBuilder
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
	
	//img create
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/texture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	wl::v1::backend::Buffer::Builder tbufferBulider(vkres->vmaAllocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	wl::v1::backend::Buffer transferBuffer = 
		tbufferBulider
		.Build();

	transferBuffer.CopyDataToBuffer(pixels);
	//el::ElBuffer buffer = vkres->CreateTransferBuffer(pixels, imageSize);

	stbi_image_free(pixels);

	wl::backend::Image::Builder imageBuilder(backend->device ,vkres->vmaAllocator, texWidth, texHeight);
	wl::backend::Image textureImage = 
		imageBuilder
		.SetImageFormat(vkRenderPresent->swapchainImageFormat)
		.SetImageUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
		.SetImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
		.SetImageViewAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
		.Build();

	vkres->TransitionImageLayout(textureImage.vkImage, vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkres->CopyBufferToImage(transferBuffer.vkBuffer, textureImage.vkImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	vkres->TransitionImageLayout(textureImage.vkImage, vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkSampler sampler = vkres->CreateTextureSampler();

	//uniform
	std::vector<wl::ElBuffer> uniformBuffers;
	vkres->CreateUniformBuffer(sizeof(wl::MVPUniform), uniformBuffers);
	std::vector<VkDescriptorSet> dscrptSets = vkres->CreateDescriptorSets(setlayout);

	wl::MVPUniform ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.model = modelMat4;
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), vkRenderPresent->surfaceCaps.currentExtent.width / (float)vkRenderPresent->surfaceCaps.currentExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	for (int i = 0; i < 2; i++)
	{
		memcpy(uniformBuffers[i].allocationInfo.pMappedData, &ubo, sizeof(ubo));
	}

	//elUpdateDscrpSets
	for (size_t i = 0; i < 2; i++)
	{
		VkDescriptorBufferInfo bufferInfo{
			.buffer = uniformBuffers[i].buffer,
			.offset = 0,
			.range = sizeof(wl::MVPUniform)
		};

		VkDescriptorImageInfo imgInfo{
			.sampler = sampler,
			.imageView = textureImage.vkImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = dscrptSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = dscrptSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imgInfo;

		vkUpdateDescriptorSets(backend->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	std::vector<wl::ElVertexPNT> vertices;
	wl::VertexIndice indices;
	LoadGLTFMeshVertices(meshFilePath, vertices, indices);

	//el::ElBuffer vertexBuffer;
	//el::ElBuffer indexBuffer;

	wl::v1::backend::Buffer::Builder bufferBulider(vkres->vmaAllocator, sizeof(wl::ElVertexPNT) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	wl::v1::backend::Buffer vertexBuffer2 = 
		bufferBulider
		.Build();

	vertexBuffer2.CopyDataToBuffer(vertices.data());

	wl::v1::backend::Buffer indexBuffer2 = 
		bufferBulider
		.SetBufferUsage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
		.Build();

	indexBuffer2.CopyDataToBuffer(indices.data());

	//vkres->CreateVertexBuffer(vertices, vertexBuffer);
	//vkres->CreateIndexBuffer(indices, indexBuffer);

	vector<uint16_t> indexCounts = { static_cast<uint16_t>(indices.size()) };
	//vector<VkBuffer> vertexBuffers = { vertexBuffer.buffer };
	vector<VkBuffer> vertexBuffers = { vertexBuffer2.vkBuffer };

	vector<VkBuffer> indexBuffers = { indexBuffer2.vkBuffer };
	vector<vector<VkDescriptorSet>> setSets{ dscrptSets };

	bool quit = false;
	SDL_Event event;
	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				quit = true;
			}
		}

		vkRenderPresent->Render(renderPass, pipeline, pipelineLayout, indexCounts, vertexBuffers, indexBuffers, setSets);

		Uint64 currentTime = SDL_GetPerformanceCounter();
		float deltaTime = (currentTime - startTime) / (float)frequency;
		startTime = currentTime;


		// º∆À„ FPS
		float fps = 1.0f / deltaTime;
		std::string titlie = std::to_string(fps);
	}

	SDL_Quit();
}