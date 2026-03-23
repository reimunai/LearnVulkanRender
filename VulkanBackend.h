#ifndef BACKEND_H
#define BACKEND_H


#pragma once
#include <vulkan/vulkan.h>
//#define VMA_STATIC_VULKAN_FUNCTIONS 0
//#define VMA_DYNAMIC_VULKAN_FUNCTION 1
//#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shaderLoader.h"
#include "VertexStruct.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>

#include <chrono>
//TODO:
//buffer device address
namespace el
{
	enum class CullMode
	{
		None = VK_CULL_MODE_NONE,
		Front = VK_CULL_MODE_FRONT_BIT,
		Back = VK_CULL_MODE_BACK_BIT,
		FrondAanBack = VK_CULL_MODE_FRONT_AND_BACK,

	};
	enum class BlendMode
	{
		None,
		Transparent,
	};
	struct VertexPC {
		glm::vec3 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexPC);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(VertexPC, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(VertexPC, color);

			return attributeDescriptions;
		}
	};

	struct VertexPN
	{
		glm::vec3 pos;
		glm::vec3 normal;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexPN);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(VertexPN, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(VertexPN, normal);

			return attributeDescriptions;
		}
	};

	/*struct VertexPNT
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VertexPNT);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(VertexPNT, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(VertexPNT, normal);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(VertexPNT, texCoord);

			return attributeDescriptions;
		}
	};*/

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	using std::vector;
	namespace backend
	{
		struct PipelineConfig
		{
			
			
			//shader
			//vertex input descriptor
			//inputassembly
				//topology mode
			//rasterizationState 
				//polygonMode
				//cullmode
				//front face
			//color blend
			uint32_t shaderID;
			CullMode cullMode;
			BlendMode blendMode;

			VkShaderModule vertexShaderModule;
			VkShaderModule fragShaderModule;
		};
		struct PipelineLayoutConfig
		{

		};

		//TODO:
		//隔离不同物体的uniform
		class VulkanBackend
		{
		public:
			//instance and device
			VkInstance instance = nullptr;
			VkPhysicalDevice phyDevice = nullptr;
			VkDevice device = nullptr;
			//queue
			VkQueue queue = nullptr;
			uint32_t queueFamilyIndex = 0;
			//window surface handle
			SDL_Window* window = nullptr;
			VkSurfaceKHR surface = nullptr;
			VkSurfaceCapabilitiesKHR surfaceCaps{};
			//present swapchain
			VkSwapchainKHR swapchain = nullptr;
			vector<VkImage> swapchainImages;
			vector<VkImageView> swapChainImageViews;
			vector<VkFramebuffer> swapChainFramebuffers;
			VkFormat swapchainImageFormat;
			VkRenderPass presentRenderPass;
			//cmd
			VkCommandPool commandPool;
			vector<VkCommandBuffer> commandBuffers;
			//sync
			vector<VkSemaphore> imageAvailableSemaphores;
			vector<VkSemaphore> renderFinishedSemaphores;
			vector<VkFence> inFlightFences;
			//depth test
			VkFormat depthFormat;
			VkImage depthImage;
			VkImageView depthImageView;
			VmaAllocation depthImageAllocation;
			//vma
			VmaAllocator allocator;

			const float queuePriorities = 1.0f;
			static const int MAX_FRAMES_IN_FLIGHT = 2;

			//extension feature
			const vector<const char*> deviceExtensions{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME 
			};
			VkPhysicalDeviceVulkan12Features enabledVk12Features{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
				.descriptorIndexing = true,
				.shaderSampledImageArrayNonUniformIndexing = true,
				.descriptorBindingVariableDescriptorCount = true,
				.runtimeDescriptorArray = true,
				.bufferDeviceAddress = true
			};
			const VkPhysicalDeviceVulkan13Features enabledVk13Features{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
				.pNext = &enabledVk12Features,
				.synchronization2 = true,
				.dynamicRendering = true,
			};
			const VkPhysicalDeviceFeatures enabledVk10Features{
				.samplerAnisotropy = VK_TRUE
			};

		public:
			uint32_t currentFrame = 0;

		public:
			void Init();
			void Cleanup();

			
			VkCommandBuffer beginSingleTimeCommands();
			void endSingleTimeCommands(VkCommandBuffer commandBuffer);

			void RecordCommandBuffer(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout, vector<uint16_t>& indexCounts,
				vector<VkBuffer>& vertexBuffers, vector<VkBuffer>& indicesBuffers, vector<vector<VkDescriptorSet>>& descriptorSets);
			void Render(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout, vector<uint16_t>& indexCounts,
				vector<VkBuffer>& vertexBuffers, vector<VkBuffer>& indexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets);

			VkRenderPass GetPresentRenderPass()
			{
				return presentRenderPass;
			}
			VkDevice GetDevive()
			{
				return device;
			}
			~VulkanBackend();

		protected:
			void initSDL();
			void creatInstance();
			void seletePhysicsDevice();
			void setupDevice();
			void InitVmaAllocator();
			void createSwapchain();

			void cleanupSwapChain();
			void recreateSwapchain();

			void createPresentRenderPass();

			void createSwapchainFramebuffers();

			void SetDepthFormat();
			void CreateDepthImage();

			void createCommandPool();
			void createCommandBuffers();

			void createSyncObjects();
		};

		class VulkanPipeline
		{
		private:
			VulkanBackend* backend;
		public:
			VulkanPipeline(VulkanBackend& backend);

			VkDescriptorSetLayout CreateDescriptorSetLayout(vector<VkDescriptorSetLayoutBinding>& bindings);
			VkPipelineLayout CreatePipelineLayout(VkDescriptorSetLayout& descriptorSetLayout);
			VkRenderPass CreateRenderPass();
			VkPipeline CreatePipeline(PipelineConfig config, VkPipelineLayout& pipelineLayout, VkRenderPass& renderPass);
		};

		//VUlkanResource like uniform&vertex buffer, image, Sets
		//will use VMA
		class VulkanMemory
		{
		private:
			VulkanBackend* backend;
			VkDescriptorPool descriptorPool;
			VkCommandBuffer transferCmdBuffer;

			//vma
			VmaAllocator allocator;
		public:
			VulkanMemory(VulkanBackend& backend);
			void CreateIndexBuffer(el::VertexIndice& indices, VkBuffer& buffer, VmaAllocation allocation, VmaAllocationInfo& allocationInfo);
			void CreateVertexBuffer(vector<ElVertexPNT>& vertices, VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo& allocationInfo);
			void CreateUniformBuffer(VkDeviceSize bufferSize, vector<VkBuffer>& buffers, vector<VkDeviceMemory>& memorys, vector<void*>& maps);
			vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, vector<VkBuffer>& uniformBuffers);
			
			VkBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
			VkDeviceMemory AllocateDeviceMemory(VkDeviceSize allocateSize, uint32_t memoryTypeIndex);
			uint32_t GetMemoryType(uint32_t memTpyeBit, VkMemoryPropertyFlags propertyFlags);
			void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		private:
			void InitVmaAllocator();
			void InitDescriptorPool();
			void InitTransferCmdBuffer();
			void Cleanup();
		};
	}
}
#endif // !BACKEND_H