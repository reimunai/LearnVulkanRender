#pragma once

#include "VulkanBackend.h"

using namespace myapplication::backend;
using std::vector;
class VulkanBackend2 : public VulkanBackend
{
public:
	//clear
	vector<VkBuffer> meshBuffers;
	vector<VkDeviceMemory> meshBufferMemorys;
	vector<uint32_t> vertexCount;
public:
	void createVertexBuffer(unsigned long long bufferSize, const void* vertexData, uint32_t vertexCount);
	
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void init()
	{
		initSDL();
		creatInstance();
		seletePhysicsDevice();
		setupDevice();

		createSwapchain();

		createRenderPass();
		createRenderPipeline();

		createFramebuffers();
		//createVertexBuffer();
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();
	}

	void render();
};