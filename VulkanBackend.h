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

		class VulkanBackend
		{
		public:

			VkInstance instance = nullptr;
			VkPhysicalDevice phyDevice = nullptr;
			VkDevice device = nullptr;

			VkQueue queue = nullptr;
			uint32_t queueFamilyIndex = 0;

			SDL_Window* window = nullptr;
			VkSurfaceKHR surface = nullptr;
			VkSurfaceCapabilitiesKHR surfaceCaps{};

			VkSwapchainKHR swapchain = nullptr;
			vector<VkImage> swapchainImages;
			vector<VkImageView> swapChainImageViews;
			vector<VkFramebuffer> swapChainFramebuffers;

			VkFormat swapchainImageFormat;

			VkCommandPool commandPool;

			vector<VkCommandBuffer> commandBuffers;

			vector<VkSemaphore> imageAvailableSemaphores;
			vector<VkSemaphore> renderFinishedSemaphores;
			vector<VkFence> inFlightFences;

			const float queuePriorities = 1.0f;
			static const int MAX_FRAMES_IN_FLIGHT = 2;

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

			//
			VkDescriptorPool descriptorPool;
			VkDescriptorSetLayout descriptorSetLayout;
			VkPipelineLayout pipelineLayout;
			VkRenderPass renderPass;
			VkPipeline pipeline;

			VkBuffer vertexBuffer;
			VkDeviceMemory vertexBufferMemory;

			std::vector<VkDeviceMemory> uniformBuffersMemory;
			std::vector<void*> uniformBuffersMapped;

			uint32_t currentFrame = 0;

		public:
			std::vector<VkBuffer> uniformBuffers;
			std::vector<VkDescriptorSet> descriptorSets;

			void init();
			void cleanup();
			void render();
			void render1(vector<VkBuffer>& buffers);

			void createVertexBuffer(VkDeviceSize bufferSize, const void* vertexData, VkBuffer& buffer, VkDeviceMemory memnry);
			void createUniformBuffer(VkDeviceSize bufferSize, VkBuffer& uniformBuffer, VkDeviceMemory& memory, void*& map);
			//will support pipeline arg
			//void recordCmdBuffer(VkCommandBuffer cmdBuffer, uint32_t imgIndex, uint32_t vertexCount);
			//VkDescriptorSetLayout createDescriptorSetLayout();


			void recordCommandBuffer1(VkCommandBuffer commandBuffer, uint32_t imageIndex, vector<VkBuffer>& vertexBuffers1);
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
			void createVertexBuffer();
			void createUniformBuffers();

			void createDescriptorPool();
			void createDescriptorSets();
			void createRenderPass();
			void createDescriptorSetLayout();
			void createRenderPipeline();

			void createFramebuffers();

			void createCommandPool();
			void createCommandBuffers();

			void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

			void createSyncObjects();

			void updateUniformBuffer(uint32_t currentImage, glm::mat4 modelMat4 = glm::mat4(1.0f));
		};



		class VulkanPipeline
		{
		private:
			VulkanBackend* backend;
		public:
			VulkanPipeline(VulkanBackend& backend);
			//~VulkanPipeline();

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
		public:
			VulkanMemory(VulkanBackend& backend);
			void CreateVertexBuffer(vector<VertexPC> vertices, VkBuffer& vertexBuffer, VkDeviceMemory& memory);
			void CreateUniformBuffer(VkDeviceSize bufferSize, vector<VkBuffer>& buffers, vector<VkDeviceMemory>& memorys, vector<void*>& maps);
			vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorSetLayout& descriptorSetLayout, vector<VkBuffer>& uniformBuffers);
		private:
			void InitDescriptorPool();
			void Cleanup();

			VkBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
			VkDeviceMemory AllocateDeviceMemory(VkDeviceSize allocateSize, uint32_t memoryTypeIndex);
			uint32_t GetMemoryType(uint32_t memTpyeBit, VkMemoryPropertyFlags propertyFlags);
		};
	}
}