#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "Component.hpp"
#include "application.h"
#include "shaderLoader.h"
#include "GltfLoader.h"
#include <string>

#include <stb_image.h>

using namespace el::backend;
using namespace el::component;
using namespace el::app;

const string meshFilePath = "meshs/mk.gltf";

auto app = new Application();
#pragma region tutorial
//need free
VkImage textureImage;
VkDeviceMemory textureImageMemory;
VkImageView textureImageView;
VkSampler textureSampler;

void elUpdateDscrpSets(std::vector<VkBuffer>& uniformBuffers, std::vector<VkDescriptorSet>& dscrptSets)
{
	for (size_t i = 0; i < app->backend->MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{
			.buffer = uniformBuffers[i],
			.offset = 0,
			.range = sizeof(el::UniformBufferObject)
		};

		VkDescriptorImageInfo imgInfo{
			.sampler = textureSampler,
			.imageView = textureImageView,
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

		vkUpdateDescriptorSets(app->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

//
VkCommandBuffer beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = app->backend->commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(app->GetDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}
//
void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(app->backend->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(app->backend->queue);

	vkFreeCommandBuffers(app->GetDevice(), app->backend->commandPool, 1, &commandBuffer);
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = 0,// TODO
		.dstAccessMask = 0, // TODO
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

		.image = image,
		.subresourceRange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},

		.imageOffset = { 0, 0, 0 },
		.imageExtent = {
			width,
			height,
			1
		},
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	endSingleTimeCommands(commandBuffer);
}

void createTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/texture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	auto stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	auto stagingFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	VkMemoryRequirements stagingRequirements;
	stagingBuffer = app->memory->CreateBuffer(imageSize, stagingUsage);
	vkGetBufferMemoryRequirements(app->backend->device, stagingBuffer, &stagingRequirements);
	stagingMemory = app->memory->AllocateDeviceMemory(stagingRequirements.size, app->memory->GetMemoryType(stagingRequirements.memoryTypeBits, stagingFlag));
	vkBindBufferMemory(app->backend->device, stagingBuffer, stagingMemory, 0);
	
	void* data;
	vkMapMemory(app->GetDevice(), stagingMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(app->GetDevice(), stagingMemory);

	stbi_image_free(pixels);

	VkImageCreateInfo imageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.extent{
			.width = static_cast<uint32_t>(texWidth),
			.height = static_cast<uint32_t>(texHeight),
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	if (vkCreateImage(app->GetDevice(), &imageInfo, NULL, &textureImage) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(app->GetDevice(), textureImage, &memRequirements);
	textureImageMemory = app->memory->AllocateDeviceMemory(memRequirements.size, app->memory->GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	vkBindImageMemory(app->GetDevice(), textureImage, textureImageMemory, 0);

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(app->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(app->GetDevice(), stagingMemory, nullptr);
} 

void createTextureImageView()
{
	//抽象为方法
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(app->GetDevice(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}

void createTextureSampler()
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(app->backend->phyDevice, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(app->GetDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}
#pragma endregion
int main()
{
	Uint64 startTime = SDL_GetPerformanceCounter();
	Uint64 frequency = SDL_GetPerformanceFrequency();

	

	auto vertShaderCode = el::resourcemanagement::ShaderLoader::readFile("shaders/vert.spv");
	auto fragShaderCode = el::resourcemanagement::ShaderLoader::readFile("shaders/frag.spv");
	VkShaderModule vertShaderModule = el::resourcemanagement::ShaderLoader::createShaderModule(vertShaderCode, app->GetDevice());
	VkShaderModule fragShaderModule = el::resourcemanagement::ShaderLoader::createShaderModule(fragShaderCode, app->GetDevice());

	PipelineConfig config = {
		.shaderID = 0,
		.cullMode = el::CullMode::None,
		.blendMode = el::BlendMode::None,
		.vertexShaderModule = vertShaderModule,
		.fragShaderModule = fragShaderModule,
	};

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
	app->InitPipelineFamily(config, binds);

	//textureImage
	createTextureImage();
	createTextureImageView();
	createTextureSampler();

	vector<VkBuffer> uniformBuffers;
	vector<VkDeviceMemory> uniformMemorys;
	vector<VkDescriptorSet> dscrptSets;
	auto size = sizeof(el::UniformBufferObject);
	vector<void*> map;
	app->memory->CreateUniformBuffer(size, uniformBuffers, uniformMemorys, map);
	dscrptSets = app->memory->CreateDescriptorSets(app->GetDescriptorSetLayout(), uniformBuffers);

	vector<VkBuffer> uniformBuffers1;
	vector<VkDeviceMemory> uniformMemorys1;
	vector<VkDescriptorSet> dscrptSets1;
	vector<void*> map1;
	app->memory->CreateUniformBuffer(size, uniformBuffers1, uniformMemorys1, map1);
	dscrptSets1 = app->memory->CreateDescriptorSets(app->GetDescriptorSetLayout(), uniformBuffers1);

	//Texture Test
	elUpdateDscrpSets(uniformBuffers, dscrptSets);
	elUpdateDscrpSets(uniformBuffers1, dscrptSets1);

	vector<vector<VkDescriptorSet>> setSets(0);
	setSets.push_back(dscrptSets);
	setSets.push_back(dscrptSets1);


	Entity e1;
	auto renderer1 = e1.AddComponent<MeshRenderer>();
	//renderer1->mesh = {
	//	{{-0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f},{0.0f, 1.0f}},
	//	{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}, {1.0f, 1.0f}},
	//	{{0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}, {1.0f, 0.0f}},
	//	{{-0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}, {0.0f, 0.0f}},
	//};
	//renderer1->indices = {
	//	0, 1, 2,
	//	0, 2, 3,
	//};
	if (LoadGLTFMeshVertices(meshFilePath, renderer1->mesh, renderer1->indices) != true)
	{
		throw std::runtime_error("failed to load gltf!");
	}
	renderer1->RenderInit(*(app->memory));

	Entity e2;
	auto renderer2 = e2.AddComponent<MeshRenderer>();
	renderer2->mesh = {
		{{-0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f},{0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}, {1.0f, 1.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}, {1.0f, 0.0f}},
		{{-0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}, {0.0f, 0.0f}},
	};
	renderer2->indices = {
		0, 1, 2,
		0, 2, 3,
	};
	renderer2->RenderInit(*(app->memory));


	vector<VkBuffer> buffers(0);
	buffers.push_back(renderer1->vertexBuffer);
	buffers.push_back(renderer2->vertexBuffer);

	vector<uint16_t> indexCounts(0);
	indexCounts.push_back(static_cast<uint16_t>(renderer1->indices.size()));
	indexCounts.push_back(static_cast<uint16_t>(renderer2->indices.size()));

	vector<VkBuffer> indexBuffers(0);
	indexBuffers.push_back(renderer1->indexBuffer);
	indexBuffers.push_back(renderer2->indexBuffer);


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

		Uint64 currentTime = SDL_GetPerformanceCounter();
		float deltaTime = (currentTime - startTime) / (float)frequency;
		startTime = currentTime;

		app->UpdateUniform(map, glm::vec3(0.0f, 0.0f, 0.0f));
		app->UpdateUniform(map1, glm::vec3(0.0f, 0.0f, -0.2f));
		app->Render(indexCounts, buffers, indexBuffers, setSets);

		// 计算 FPS
		float fps = 1.0f / deltaTime;
		std::string titlie = std::to_string(fps);
		//SDL_SetWindowTitle(engine->window, titlie.c_str());
	}

	SDL_Quit();
}