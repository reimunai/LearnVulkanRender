#include "Test.h"

#include <iostream>

#include "VkBackend.h"
#include "VkRender.h"
#include "VkResources.h"
#include "DescriptorAllocator.h"
#include "RenderPassBuilder.h"
#include "shaderLoader.h"
#include "PipelineBuilder.h"
#include "stb_image.h"
#include "CubeImageFactor.h"
#include "SamplerBuilder.h"
#include "Mesh.h"
#include "Buffer.h"
#include "FrameBufferBuilder.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <chrono>
using namespace wl::backend;
namespace test
{
	void Test_Run()
	{
		using wl::backend::PipelineBuilder;
		using wl::resource::CubeImageFactor;
		using wl::backend::SamplerBuilder;
		using wl::component::Mesh;
		//Init
		SDL_Window* window;
		VkBackend* backend = nullptr;
		VkRenderPresent* vkRenderPresent = nullptr;
		VmaAllocator allocator;
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			std::cerr << "failed to init SDL" << std::endl;
			return;
		}

		window = SDL_CreateWindow("Vulkan", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
		backend = new VkBackend();
		VmaAllocatorCreateInfo vmaCI{
		.physicalDevice = backend->phyDevice,
		.device = backend->device,
		.instance = backend->instance,
		};
		if (vmaCreateAllocator(&vmaCI, &allocator) != VK_SUCCESS)
		{
			throw std::runtime_error("VMA allocator creation failed in VkResources::InitVma");
		}
		vkRenderPresent = new VkRenderPresent(*backend, allocator, window);
		DescriptorAllocator* dscrptAllocator = new DescriptorAllocator(backend->device, 2);

		std::vector<VkDescriptorSetLayout> dscrptorSetlayouts{
			dscrptAllocator->ObjectUniformLayout,
			dscrptAllocator->ResourcesUniformLayout
		};
		auto pipelineLayout = dscrptAllocator->CreatePipelineLayout(dscrptorSetlayouts);
		
		std::vector<VkAttachmentReference> attachmentRefs{
			{
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}
		};
		auto presentRenderPass =
			wl::backend::RenderPassBuilder(backend->device)
			.AddAttachment(vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			.AddSubpass(
				{
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.colorAttachmentCount = 1,
					.pColorAttachments = attachmentRefs.data()
				}
			)
			.Build();

		auto skyboxPipeline = CreateSkyBoxPipeline(backend, pipelineLayout, presentRenderPass);
		auto lambertPipeline = CreateLambertPipeline(backend, pipelineLayout, presentRenderPass);

		vector<string> cubetexFilePath{
		"textures/skybox/right.jpg",
		"textures/skybox/left.jpg",
		"textures/skybox/top.jpg",
		"textures/skybox/bottom.jpg",
		"textures/skybox/front.jpg",
		"textures/skybox/back.jpg",
		};
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(cubetexFilePath[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;
		if (!pixels)
		{
			throw std::runtime_error("failed to load texture image!");
		}
		stbi_image_free(pixels);
		
		auto cubeImage = CubeImageFactor::CreateCubeImage(backend->device, allocator, texWidth, texHeight);
		CubeImageFactor::CopyTextureToCubeImage(cubeImage, cubetexFilePath, allocator, *backend);
		auto cubeSampler = SamplerBuilder(*backend).Builder();

		string meshFilePath = "meshs/cube10.gltf";
		wl::component::Mesh cubeMesh(meshFilePath, allocator);

		string sphereFilePath = "meshs/ball_0.gltf";
		wl::component::Mesh sphereMesh(sphereFilePath, allocator);

		using wl::v1::backend::Buffer;
		vector<Buffer> uniformBuffers;
		wl::MVPUniform ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.view = glm::lookAt(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), vkRenderPresent->surfaceCaps.currentExtent.width / (float)vkRenderPresent->surfaceCaps.currentExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		for (int i = 0; i < 2; i++)
		{
			uniformBuffers.push_back(Buffer::Builder(allocator, sizeof(wl::MVPUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
				.SetMemoryFlag(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
				.Build());

			uniformBuffers[i].CopyDataToBuffer(&ubo);
		}

		FrameBufferBuilder frameBufferBuilder(backend->device, presentRenderPass, 800, 600);
		vector<VkFramebuffer> framebuffers(2);
		for (int i = 0; i < 2; i++)
		{
			//TIPS:×¢ÒâË³Ðò
			vector<VkImageView> attachments = {
				vkRenderPresent->swapChainImageViews[i],
			};

			framebuffers[i] =
				frameBufferBuilder
				.SetAttackments(attachments)
				.Build();
		}

		auto ObjDscrt = dscrptAllocator->AllocateDescriptorSets(DescriptorAllocator::SETTYPE::OBJECT_UNIFORM);
		auto ImageDscrt = dscrptAllocator->AllocateDescriptorSets(DescriptorAllocator::SETTYPE::RESOURCES_UNIFORM);
		for (int i = 0; i < 2; i++)
		{
			VkDescriptorBufferInfo bufferInfo{
				.buffer = uniformBuffers[i].vkBuffer,
				.offset = 0,
				.range = sizeof(wl::MVPUniform)
			};

			VkDescriptorImageInfo imgInfo{
				.sampler = cubeSampler,
				.imageView = cubeImage.vkImageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			vector<VkWriteDescriptorSet> descriptorWrites(2);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = ObjDscrt[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = ImageDscrt[0];
			descriptorWrites[1].dstBinding = 0;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imgInfo;

			vkUpdateDescriptorSets(backend->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		auto startTime1 = std::chrono::high_resolution_clock::now();
		Uint64 startTime = SDL_GetPerformanceCounter();
		Uint64 frequency = SDL_GetPerformanceFrequency();
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

			auto currentTime1 = std::chrono::high_resolution_clock::now();
			float time1 = std::chrono::duration<float, std::chrono::seconds::period>(currentTime1 - startTime1).count();
			//TODO: multiply subpass render
			vkRenderPresent->WaitReady();
			vkRenderPresent->ResetCmd();
			vkRenderPresent->InitRecordCmdBuffer();

			std::vector<VkClearValue> clearValues(1);
			clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

			vkRenderPresent->BeginRenderPassCmd(presentRenderPass, clearValues, framebuffers);
			vkRenderPresent->BindPipelineCmd(skyboxPipeline);
			vkRenderPresent->BindVertexCmd(cubeMesh.vertexBuffer.vkBuffer);
			vkRenderPresent->BindIndexCmd(cubeMesh.indiceBuffer.vkBuffer);
			vector<VkDescriptorSet> sets{ ObjDscrt[vkRenderPresent->currentFrame], ImageDscrt[0] };
			//vkRenderPresent->BindDescriptorSets(pipelineLayout, sets);
			vkCmdBindDescriptorSets(
				vkRenderPresent->commandBuffers[vkRenderPresent->currentFrame],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0,
				2,
				sets.data(),
				0,
				nullptr
			);
			vkRenderPresent->IndexDrawCmd(cubeMesh.indices.size());
			//sphere
			vkRenderPresent->BindPipelineCmd(lambertPipeline);
			vkRenderPresent->BindVertexCmd(sphereMesh.vertexBuffer.vkBuffer);
			vkRenderPresent->BindIndexCmd(sphereMesh.indiceBuffer.vkBuffer);
			vkCmdBindDescriptorSets(
				vkRenderPresent->commandBuffers[vkRenderPresent->currentFrame],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0,
				2,
				sets.data(),
				0,
				nullptr
			);
			vkRenderPresent->IndexDrawCmd(sphereMesh.indices.size());

			vkRenderPresent->EndRenderPassCmd();
			vkRenderPresent->EndRecordCmdBuffer();

			vkRenderPresent->Render();

			Uint64 currentTime = SDL_GetPerformanceCounter();
			float deltaTime = (currentTime - startTime) / (float)frequency;
			startTime = currentTime;
			// ¼ÆËã FPS
			float fps = 1.0f / deltaTime;
			std::string titlie = std::to_string(fps);

			//ubo.model = glm::rotate(glm::mat4(1.0f), time1 * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		/*	glm::mat4 rotateM = glm::rotate(glm::mat4(1.0f), time1 * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec4 cen(0.0f, -0.2f, 1.0f, 1.0f);
			cen = rotateM * cen;
			ubo.view = glm::lookAt(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(cen.x, cen.y, cen.z), glm::vec3(0.0f, 1.0f, 0.0f));
			memcpy(uniformBuffers[vkRenderPresent->currentFrame].allocationInfo.pMappedData, &ubo, sizeof(wl::MVPUniform));*/
		}

		SDL_Quit();
		vkDeviceWaitIdle(backend->device);
		delete dscrptAllocator;
	}

	VkPipeline CreateSkyBoxPipeline(wl::backend::VkBackend* backend, VkPipelineLayout& pipelineLayout, VkRenderPass& presentRenderPass)
	{
		using wl::resourcemanagement::ShaderLoader;
		auto shaderCode = ShaderLoader::readFile("shaders/skybox_vert.spv");
		auto vertShaderModule = ShaderLoader::createShaderModule(shaderCode, backend->device);

		shaderCode = ShaderLoader::readFile("shaders/skybox_frag.spv");
		auto fragShaderModule = ShaderLoader::createShaderModule(shaderCode, backend->device);

		auto skyboxPipeline =
			PipelineBuilder(backend->device, pipelineLayout, presentRenderPass)
			.SetVertexShasderModule(vertShaderModule)
			.SetFragShaderModule(fragShaderModule)
			.SetVertexInputBindingDescription(wl::ElVertexPNT::getBindingDescription())
			.SetAttributeDescriptions(wl::ElVertexPNT::getAttributeDescriptions())
			.SetBlendMode(wl::BlendMode::None)
			.SetCullMode(wl::CullMode::None)
			.SetColorAttachmentCount(1)
			.SetDepthTest(false)
			.Build();

		vkDestroyShaderModule(backend->device, vertShaderModule, nullptr);
		vkDestroyShaderModule(backend->device, fragShaderModule, nullptr);

		return skyboxPipeline;
	}

	VkPipeline CreateLambertPipeline(wl::backend::VkBackend* backend, VkPipelineLayout& pipelineLayout, VkRenderPass& presentRenderPass)
	{
		using wl::resourcemanagement::ShaderLoader;
		auto shaderCode = ShaderLoader::readFile("shaders/vert.spv");
		auto vertShaderModule = ShaderLoader::createShaderModule(shaderCode, backend->device);

		shaderCode = ShaderLoader::readFile("shaders/frag.spv");
		auto fragShaderModule = ShaderLoader::createShaderModule(shaderCode, backend->device);

		auto skyboxPipeline =
			PipelineBuilder(backend->device, pipelineLayout, presentRenderPass)
			.SetVertexShasderModule(vertShaderModule)
			.SetFragShaderModule(fragShaderModule)
			.SetVertexInputBindingDescription(wl::ElVertexPNT::getBindingDescription())
			.SetAttributeDescriptions(wl::ElVertexPNT::getAttributeDescriptions())
			.SetBlendMode(wl::BlendMode::None)
			.SetCullMode(wl::CullMode::Front)
			.SetColorAttachmentCount(1)
			.SetDepthTest(false)
			.Build();

		vkDestroyShaderModule(backend->device, vertShaderModule, nullptr);
		vkDestroyShaderModule(backend->device, fragShaderModule, nullptr);

		return skyboxPipeline;
	}
}