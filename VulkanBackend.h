#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
namespace myapplication
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

			void init();
			void cleanup();
			void recordCommandBuffer(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout,
				vector<VkBuffer>& vertexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets);
			void render(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout,
				vector<VkBuffer>& vertexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets);
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
			void createSwapchain();

			void cleanupSwapChain();
			void recreateSwapchain();

			uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
			void createBuffer(
				VkDeviceSize size,
				VkBufferUsageFlags usage,
				VkMemoryPropertyFlags properties,
				VkBuffer& buffer,
				VkDeviceMemory& bufferMemory);

			void createPresentRenderPass();

			void createSwapchainFramebuffers();

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

		class VulkanMemory
		{
		private:
			VulkanBackend* backend;
			VkDescriptorPool descriptorPool;
			VkCommandBuffer transferCmdBuffer;
		public:
			VulkanMemory(VulkanBackend& backend);
			void CreateVertexBuffer(vector<VertexPC> vertices, VkBuffer& vertexBuffer, VkDeviceMemory& memory);
			void CreateUniformBuffer(VkDeviceSize bufferSize, vector<VkBuffer>& buffers, vector<VkDeviceMemory>& memorys, vector<void*>& maps);
			vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, vector<VkBuffer>& uniformBuffers);
		private:
			void InitDescriptorPool();
			void InitTransferCmdBuffer();
			void Cleanup();

			VkBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
			VkDeviceMemory AllocateDeviceMemory(VkDeviceSize allocateSize, uint32_t memoryTypeIndex);
			uint32_t GetMemoryType(uint32_t memTpyeBit, VkMemoryPropertyFlags propertyFlags);
			void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		};
	}
}