#include "skyBoxTest.h"
#include "VkBackend.h"
#include "VkRender.h"
#include "VkResources.h"
#include "RenderPassBuilder.h"
#include "PipelineBuilder.h"
#include "Buffer.h"
#include "Image.h"
#include "DescriptorAllocator.h"
#include "shaderLoader.h"
#include "GltfLoader.h"

#include <iostream>
#include <vector>
#include <string>
#include <array>

#include <stb_image.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include "FrameBufferBuilder.h"


using std::vector;
using std::string;
using wl::v1::backend::Buffer;
using wl::backend::RenderPassBuilder;
using wl::backend::Image;
using wl::resourcemanagement::ShaderLoader;
using wl::backend::PipelineBuilder;
using namespace wl::backend;
void test::Sky_Box_Run()
{
	//TODO:智能指针管理
	SDL_Window* window = nullptr;
	VkBackend* backend = nullptr;
	VkResources* vkres = nullptr;
	VkRenderPresent* vkRenderPresent = nullptr;
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "failed to init SDL" << std::endl;
		return;
	}

	window = SDL_CreateWindow("Vulkan", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	backend = new VkBackend();
	vkres = new VkResources(*backend);
	vkRenderPresent = new VkRenderPresent(*backend, vkres->vmaAllocator, window);
	DescriptorAllocator* dscrptAllocator = new DescriptorAllocator(backend->device, 2);


	
	vector<string> cubetexFilePath{
		"textures/立方体贴图/cube_back_suburban_garden_2k.jpg",
		"textures/立方体贴图/cube_bottom_suburban_garden_2k.jpg",
		"textures/立方体贴图/cube_front_suburban_garden_2k.jpg",
		"textures/立方体贴图/cube_left_suburban_garden_2k.jpg",
		"textures/立方体贴图/cube_right_suburban_garden_2k.jpg",
		"textures/立方体贴图/cube_top_suburban_garden_2k.jpg",
	};
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(cubetexFilePath[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	Buffer tbuffer =
		Buffer::Builder(vkres->vmaAllocator, 6 * imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
		.Build();

	void* pData = nullptr;
	vmaMapMemory(tbuffer.allocator, tbuffer.allocation, &pData);
	memcpy(pData, pixels, imageSize);

	for (int i = 1; i < 6; i++)
	{
		pixels = stbi_load(cubetexFilePath[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		if (!pixels)
		{
			throw std::runtime_error("failed to load texture image!");
		}
		memcpy((uint8_t*)pData + imageSize * i, pixels, imageSize);
	}
	vmaFlushAllocation(tbuffer.allocator, tbuffer.allocation, 0, 6 * imageSize);
	vmaUnmapMemory(tbuffer.allocator, tbuffer.allocation);

	Image cubeImage =
		Image::Builder(backend->device, vkres->vmaAllocator, texWidth, texHeight)
		.SetImageFlag(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
		.SetImageFormat(VK_FORMAT_R8G8B8A8_SRGB)
		.SetImageUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
		.SetImageInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
		.SetImageViewAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
		.SetArrayLayers(6)
		.SetViewType(VK_IMAGE_VIEW_TYPE_CUBE)
		.Build();


	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(backend->phyDevice, &properties);
	VkSamplerCreateInfo samplerInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};
	VkSampler texSampler;

	if (vkCreateSampler(backend->device, &samplerInfo, nullptr, &texSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

		.image = cubeImage.vkImage,
		.subresourceRange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 6,
		},
	};
	auto cmd = backend->BeginOneTimesCommands();
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	std::array<VkBufferImageCopy, 6>copyRegion{};
	for (int i = 0; i < 6; i++)
	{
		copyRegion[i] = {
			.bufferOffset = i * imageSize,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,

			.imageSubresource{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = static_cast<uint32_t>(i),
				.layerCount = 1
			},

			.imageOffset = {0, 0, 0},
			.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1}
		};
	}
	vkCmdCopyBufferToImage(cmd, tbuffer.vkBuffer, cubeImage.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, copyRegion.data());

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	backend->EndOneTimesCommands(cmd);

	vector<VkDescriptorSetLayout> dscrptSetLayouts{
		//dscrptAllocator->globalSetLayout,
		dscrptAllocator->ObjectUniformLayout,
		dscrptAllocator->ResourcesUniformLayout
	};
	auto pipelineLayout = dscrptAllocator->CreatePipelineLayout(dscrptSetLayouts);

	vector<VkAttachmentReference> attachmentRefs{
		{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		}
	};
	auto renderpass =
		RenderPassBuilder(backend->device)
		.AddAttachment(vkRenderPresent->swapchainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		.AddSubpass(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = attachmentRefs.data()
			}
		)
		.Build();

	auto shaderCode = ShaderLoader::readFile("shaders/skybox_vert.spv");
	auto vertShaderModule = ShaderLoader::createShaderModule(shaderCode, backend->device);

	shaderCode = ShaderLoader::readFile("shaders/skybox_frag.spv");
	auto fragShaderModule = ShaderLoader::createShaderModule(shaderCode, backend->device);

	auto pipeline =
		PipelineBuilder(backend->device, pipelineLayout, renderpass)
		.SetVertexShasderModule(vertShaderModule)
		.SetFragShaderModule(fragShaderModule)
		.SetVertexInputBindingDescription(wl::ElVertexPNT::getBindingDescription())
		.SetAttributeDescriptions(wl::ElVertexPNT::getAttributeDescriptions())
		.SetBlendMode(wl::BlendMode::None)
		.SetCullMode(wl::CullMode::None)
		.SetColorAttachmentCount(1)
		.SetDepthTest(false)
		.Build();

	string meshFilePath = "meshs/cube10.gltf";
	vector<wl::ElVertexPNT> vertices;
	wl::VertexIndice indices;
	LoadGLTFMeshVertices(meshFilePath, vertices, indices);

	Buffer vertexBuffer =
		Buffer::Builder(vkres->vmaAllocator, sizeof(wl::ElVertexPNT) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		.Build();

	vertexBuffer.CopyDataToBuffer(vertices.data());

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

	vector<Buffer> uniformBuffers;
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

	FrameBufferBuilder frameBufferBuilder(backend->device, renderpass, 800, 600);
	vector<VkFramebuffer> framebuffers(2);
	for (int i = 0; i < 2; i++)
	{
		//TIPS:注意顺序
		vector<VkImageView> attachments = {
			vkRenderPresent->swapChainImageViews[i],
		};

		framebuffers[i] =
			frameBufferBuilder
			.SetAttackments(attachments)
			.Build();
	}

	auto Objdscrt = dscrptAllocator->AllocateDescriptorSets(DescriptorAllocator::SETTYPE::OBJECT_UNIFORM);
	auto ImageDscrt = dscrptAllocator->AllocateDescriptorSets(DescriptorAllocator::SETTYPE::RESOURCES_UNIFORM);
	for (int i = 0; i < 2; i++)
	{
		VkDescriptorBufferInfo bufferInfo{
			.buffer = uniformBuffers[i].vkBuffer,
			.offset = 0,
			.range = sizeof(wl::MVPUniform)
		};

		VkDescriptorImageInfo imgInfo{
			.sampler = texSampler,
			.imageView = cubeImage.vkImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		vector<VkWriteDescriptorSet> descriptorWrites(2);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = Objdscrt[i];
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

		//TODO:for multiply subpass render
		vkRenderPresent->WaitReady();
		vkRenderPresent->ResetCmd();
		vkRenderPresent->InitRecordCmdBuffer();

		std::vector<VkClearValue> clearValues(1);
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

		vkRenderPresent->BeginRenderPassCmd(renderpass, clearValues, framebuffers);
		vkRenderPresent->BindPipelineCmd(pipeline);
		vkRenderPresent->BindVertexCmd(vertexBuffer.vkBuffer);
		vkRenderPresent->BindIndexCmd(indexBuffer.vkBuffer);
		vector<VkDescriptorSet> sets{Objdscrt[vkRenderPresent->currentFrame], ImageDscrt[0]};
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
		vkRenderPresent->IndexDrawCmd(indices.size());
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
