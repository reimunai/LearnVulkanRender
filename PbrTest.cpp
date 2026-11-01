#include "PbrTest.h"
#include "VkBackend.h"
#include "VkRender.h"
#include "VkResources.h"
#include "shaderLoader.h"

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Buffer.h"
#include "PipelineBuilder.h"
#include "RenderPassBuilder.h"
#include "GltfLoader.h"
#include "FrameBufferBuilder.h"
#include "Image.h"

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

	VkShaderModule gbufferVertShaderModule, gbufferFragShaderModule;
	VkShaderModule lightVertShaderModule, lightFragShaderModule;


	VkRenderPass renderPass;
	VkPipeline gbufferPipeline;
	VkPipeline lightPipeline;



	const string meshFilePath = "meshs/Ball_2.gltf";
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

	static void ConfigRenderPass()
	{
		vector<VkAttachmentReference> gbufferColorAttachmentRefs{
			{
				//out albedo
				.attachment = 2,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			},
			{
				//out normal
				.attachment = 3,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			},
			{
				//out pos
				.attachment = 4,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}
		};
		vector<VkAttachmentReference> lightInputAttachmentRefs{
			{
				// albedo
				.attachment = 2,
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			},
			{
				// normal
				.attachment = 3,
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			},
			{
				//pos
				.attachment = 4,
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			},
		};

		VkAttachmentReference lightOutColorAttachmentRef{
			//out present
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		VkAttachmentReference depthAttachmentRef{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		renderPass = RenderPassBuilder(backend->device)
			//out present
			.AddAttachment(vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			//depthtest
			.AddAttachment(vkRenderPresent->GetSuitalbeDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			//out albedo
			.AddAttachment(vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			//out normal
			.AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			//out view pos
			.AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.AddSubpass(
				{
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.colorAttachmentCount = static_cast<uint32_t>(gbufferColorAttachmentRefs.size()),
					.pColorAttachments = gbufferColorAttachmentRefs.data(),
					.pDepthStencilAttachment = &depthAttachmentRef
				}
			)
			.AddSubpass(
				{
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.inputAttachmentCount = static_cast<uint32_t>(lightInputAttachmentRefs.size()),
					.pInputAttachments = lightInputAttachmentRefs.data(),
					.colorAttachmentCount = 1,
					.pColorAttachments = &lightOutColorAttachmentRef
				}
			)
			.AddDependency(
				{
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
					.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				}
				)
			.AddDependency(
				{
					.srcSubpass = 0,
					.dstSubpass = 1,
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
				}
			)
			.Build();
	}

	static void ConfigGBufferPipeline(VkPipelineLayout& pipelineLayout)
	{

		gbufferPipeline =
			PipelineBuilder(backend->device, pipelineLayout, renderPass)
			.SetVertexShasderModule(gbufferVertShaderModule)
			.SetFragShaderModule(gbufferFragShaderModule)
			.SetVertexInputBindingDescription(wl::ElVertexPNT::getBindingDescription())
			.SetAttributeDescriptions(wl::ElVertexPNT::getAttributeDescriptions())
			.SetBlendMode(wl::BlendMode::None)
			.SetCullMode(wl::CullMode::Back)
			//TODO:
			.SetColorAttachmentCount(3)
			.SetDepthTest(true)
			.SetDepthWrite(true)
			.SetDepthCompareOp(VK_COMPARE_OP_LESS)
			.Build();
	}
	static void ConfigLightPipeline(VkPipelineLayout& pipelineLayout)
	{
		lightPipeline =
			PipelineBuilder(backend->device, pipelineLayout, renderPass)
			.SetVertexShasderModule(lightVertShaderModule)
			.SetFragShaderModule(lightFragShaderModule)
			.EnableVertexInput(false)
			.SetBlendMode(wl::BlendMode::None)
			.SetCullMode(wl::CullMode::None)
			//TODO:
			.SetColorAttachmentCount(1)
			.SetDepthTest(false)
			.SetDepthWrite(false)
			.SetDepthCompareOp(VK_COMPARE_OP_LESS)
			.SetTargetSubpass(1)
			.Build();
	}
	static void ConfigUniformBuffer(std::vector<wl::v1::backend::Buffer>& uniformBuffers)
	{
		wl::MVPUniform ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.model = modelMat4;
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), vkRenderPresent->surfaceCaps.currentExtent.width / (float)vkRenderPresent->surfaceCaps.currentExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		for (int i = 0; i < 2; i++)
		{
			uniformBuffers.push_back(Buffer::Builder(vkres->vmaAllocator, sizeof(wl::MVPUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
				.Build());

			uniformBuffers[i].CopyDataToBuffer(&ubo);
		}
	}
	static Image CreateTextureImage(const string filePath)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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

		wl::backend::Image::Builder imageBuilder(backend->device, vkres->vmaAllocator, texWidth, texHeight);
		wl::backend::Image textureImage =
			imageBuilder
			.SetImageFormat(vkRenderPresent->swapchainImageFormat)
			.SetImageUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.SetImageInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
			.SetImageViewAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.Build();

		vkres->TransitionImageLayout(textureImage.vkImage, vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkres->CopyBufferToImage(transferBuffer.vkBuffer, textureImage.vkImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		vkres->TransitionImageLayout(textureImage.vkImage, vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		return textureImage;
	}
}
void test::Pbr_Test_Run()
{
	if (!InitWindowAndVulkan())
	{
		return;
	}

	Uint64 startTime = SDL_GetPerformanceCounter();
	Uint64 frequency = SDL_GetPerformanceFrequency();

	LoadSahderCode("shaders/gbuffer_vert.spv", gbufferVertShaderModule);
	LoadSahderCode("shaders/gbuffer_frag.spv", gbufferFragShaderModule);

	LoadSahderCode("shaders/light_vert.spv", lightVertShaderModule);
	LoadSahderCode("shaders/light_frag.spv", lightFragShaderModule);

	//TODO:
	ConfigRenderPass();
	vector<VkDescriptorSetLayoutBinding> gbufferPipelineBinds{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,//no array
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr  // Optional
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		}
	};
	auto gbufferSetlayout = vkres->CreateDescriptorSetLayout(gbufferPipelineBinds);
	auto gbufferPipelineLayout = vkres->CreatePipelineLayout(gbufferSetlayout);

	vector< VkDescriptorSetLayoutBinding> lightPipelineBinds{
		{
			//albedo
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.descriptorCount = 1,//no array
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr  // Optional
		},
		{
			//normal
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.descriptorCount = 1,//no array
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr  // Optional
		},
		{
			//pos
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.descriptorCount = 1,//no array
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr  // Optional
		}
	};
	auto lightSetLayout = vkres->CreateDescriptorSetLayout(lightPipelineBinds);
	auto lightPipelineLayout = vkres->CreatePipelineLayout(lightSetLayout);

	//config pipeline
	ConfigGBufferPipeline(gbufferPipelineLayout);
	ConfigLightPipeline(lightPipelineLayout);

	vector<wl::ElVertexPNT> vertices;
	vector<wl::VertexIndice> indices;
	LoadGLTFMeshVertices(meshFilePath, vertices, indices);

	//el::ElBuffer vertexBuffer;
	//el::ElBuffer indexBuffer;

	Buffer vertexBuffer =
		Buffer::Builder(vkres->vmaAllocator, sizeof(wl::ElVertexPNT) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		.Build();

	vertexBuffer.CopyDataToBuffer(vertices.data());

	//
	size_t indexByteSize = indices.size() * sizeof(indices[0]);
	//size_t indexByteSize = sizeof(vector<uint32_t>) * indices.size();
	//sizeof(vector<T>)为vector对象对象，和存储数据T无关
	if (indexByteSize == 0) {
		throw std::runtime_error("No index data loaded.");
	}
	Buffer indexBuffer =
		Buffer::Builder(vkres->vmaAllocator, indexByteSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
		.Build();

	indexBuffer.CopyDataToBuffer(indices.data());

	//uniformBuffer
	vector<Buffer> uniformBuffers;
	auto gbufferDscrptSets = vkres->CreateDescriptorSets(gbufferSetlayout);
	auto lightDscrptSets = vkres->CreateDescriptorSets(lightSetLayout);
	ConfigUniformBuffer(uniformBuffers);

	//texture
	Image albedoTexture = CreateTextureImage("textures/立方体贴图/cube_back_suburban_garden_2k.jpg");
	VkSampler albbedoTextureSampler = vkres->CreateTextureSampler();

#pragma region Config frame buffer
	//config frame buffer
	auto ImageBuilder = Image::Builder(backend->device, vkres->vmaAllocator, 800, 600);
	auto albedoImage =
		ImageBuilder
		.SetImageFormat(vkRenderPresent->swapchainImageFormat)
		.SetImageUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
		.SetImageInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
		.SetImageViewAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
		.Build();

	auto normalImage =
		ImageBuilder
		.SetImageFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
		.SetImageUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
		.SetImageInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
		.SetImageViewAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
		.Build();

	auto posImage =
		ImageBuilder
		.SetImageFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
		.SetImageUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
		.SetImageInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
		.SetImageViewAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
		.Build();

	auto depthImage =
		ImageBuilder
		.SetImageFormat(vkRenderPresent->GetSuitalbeDepthFormat())
		.SetImageUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		.SetImageInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
		.SetImageViewAspectMask(VK_IMAGE_ASPECT_DEPTH_BIT)
		.Build();

	FrameBufferBuilder frameBufferBuilder(backend->device, renderPass, 800, 600);
	vector<VkFramebuffer> framebuffers(2);
	for (int i = 0; i < 2; i++)
	{
		//TIPS:注意顺序
		vector<VkImageView> attachments = {
			vkRenderPresent->swapChainImageViews[i],
			depthImage.vkImageView,
			albedoImage.vkImageView,
			normalImage.vkImageView,
			posImage.vkImageView
		};

		framebuffers[i] =
			frameBufferBuilder
			.SetAttackments(attachments)
			.Build();
	}
#pragma endregion


	//UpdateDscrpSets
#pragma region UpdateDscrpSets

	for (size_t i = 0; i < 2; i++)
	{
		VkDescriptorBufferInfo bufferInfo{
			.buffer = uniformBuffers[i].vkBuffer,
			.offset = 0,
			.range = sizeof(wl::MVPUniform)
		};

		VkDescriptorImageInfo imgInfo{
			.sampler = albbedoTextureSampler,
			.imageView = albedoTexture.vkImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		VkDescriptorImageInfo albedoInfo{
			.imageView = albedoImage.vkImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		VkDescriptorImageInfo normalInfo{
			.imageView = normalImage.vkImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		VkDescriptorImageInfo posInfo{
			.imageView = posImage.vkImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		vector<VkWriteDescriptorSet> descriptorWrites(5);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = gbufferDscrptSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = gbufferDscrptSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imgInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = lightDscrptSets[i];
		descriptorWrites[2].dstBinding = 0;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &albedoInfo;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = lightDscrptSets[i];
		descriptorWrites[3].dstBinding = 1;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &normalInfo;

		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = lightDscrptSets[i];
		descriptorWrites[4].dstBinding = 2;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pImageInfo = &posInfo;

		vkUpdateDescriptorSets(backend->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
#pragma endregion
	//TODO:ClearColor 配置
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

		//TODO:for multiply subpass render
		vkRenderPresent->WaitReady();
		vkRenderPresent->ResetCmd();
		vkRenderPresent->InitRecordCmdBuffer();

		std::vector<VkClearValue> clearValues(5);
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[4].color = { 0.0f, 0.0f, 0.0f, 1.0f };

		vkRenderPresent->BeginRenderPassCmd(renderPass, clearValues, framebuffers);
		vkRenderPresent->BindPipelineCmd(gbufferPipeline);
		vkRenderPresent->BindVertexCmd(vertexBuffer.vkBuffer);
		vkRenderPresent->BindIndexCmd(indexBuffer.vkBuffer);
		vkRenderPresent->BindDescriptorSets(gbufferPipelineLayout, gbufferDscrptSets);
		vkRenderPresent->IndexDrawCmd(indices.size());
		vkRenderPresent->NextSubPass();
		vkRenderPresent->BindPipelineCmd(lightPipeline);
		vkRenderPresent->BindDescriptorSets(lightPipelineLayout, lightDscrptSets);
		vkRenderPresent->DrawCmd(6);
		vkRenderPresent->EndRenderPassCmd();
		vkRenderPresent->EndRecordCmdBuffer();

		vkRenderPresent->Render();

		Uint64 currentTime = SDL_GetPerformanceCounter();
		float deltaTime = (currentTime - startTime) / (float)frequency;
		startTime = currentTime;

		// 计算 FPS
		float fps = 1.0f / deltaTime;
		std::string titlie = std::to_string(fps);
	}

	SDL_Quit();
}