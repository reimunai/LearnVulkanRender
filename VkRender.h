#pragma once
//render and present
#ifndef VKRENDER_H
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "VkBackend.h"

#include <vector>
namespace wl
{
	namespace backend
	{
		
		class VkRenderPresent
		{
		public:
			//window surface handle
			//SDL_Window* window = nullptr;
			VkSurfaceKHR surface = nullptr;
			VkSurfaceCapabilitiesKHR surfaceCaps{};
			//present swapchain
			VkSwapchainKHR swapchain = nullptr;
			std::vector<VkImage> swapchainImages;
			std::vector<VkImageView> swapChainImageViews;
			std::vector<VkFramebuffer> swapChainFramebuffers;

			VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
			VkRenderPass presentRenderPass;
			//depth test
			VkFormat depthFormat;
			VkImage depthImage;
			VkImageView depthImageView;
			VmaAllocation depthImageAllocation;
			//cmd
			VkCommandPool commandPool;
			std::vector<VkCommandBuffer> commandBuffers;
			//sync
			std::vector<VkSemaphore> imageAvailableSemaphores;
			std::vector<VkSemaphore> renderFinishedSemaphores;
			std::vector<VkFence> inFlightFences;
			//vma
			VmaAllocator vmaAllocator;

			int currentFrame = 0;
		private:
			int maxFramesInFlight = 2;
			VkBackend* vkBackend;

		public:
			VkRenderPresent(VkBackend& backend, VmaAllocator vmaAllocator, SDL_Window* window);
		private:
			//void InitSDL(const char* title, int w, int h);
			void CreateSurface(SDL_Window* window);

			void SetDepthFormat();
			void CreateDepthImage();

			void CreateSwapchain();
			void CreateSwapchainFramebuffers();
			void RecreateSwapchain();
			void CleanupSwapChain();
			void CreatePresentRenderPass();

			void CreateCommandPool();
			void CreateCommandBuffers();
			void CreateSyncObjects();
		public:
			void RecordCommandBuffer(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout, std::vector<uint16_t>& indexCounts,
				std::vector<VkBuffer>& vertexBuffers, std::vector<VkBuffer>& indicesBuffers, std::vector<std::vector<VkDescriptorSet>>& descriptorSets);
			void Render(VkRenderPass renderPass, VkPipeline pipeline, VkPipelineLayout pipelineLayout, std::vector<uint16_t>& indexCounts,
				std::vector<VkBuffer>& vertexBuffers, std::vector<VkBuffer>& indexBuffers, std::vector<std::vector<VkDescriptorSet>>& descriptorSets);
		};
	}
}
#endif // !VKRENDER_H