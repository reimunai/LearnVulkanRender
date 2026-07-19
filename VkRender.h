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
			VkSurfaceKHR surface = nullptr;
			VkSurfaceCapabilitiesKHR surfaceCaps{};
			//present swapchain
			VkSwapchainKHR swapchain = nullptr;
			std::vector<VkImage> swapchainImages;
			std::vector<VkImageView> swapChainImageViews;

			VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
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

			void CreateSwapchain();
			void RecreateSwapchain();
			void CleanupSwapChain();

			void CreateCommandPool();
			void CreateCommandBuffers();
			void CreateSyncObjects();
		public:
			VkFormat GetSuitalbeDepthFormat();
			void InitRecordCmdBuffer();
			void BeginRenderPassCmd(VkRenderPass& renderPass, std::vector<VkClearValue>& clearValues, std::vector<VkFramebuffer>& frameBuffers);
			void BindPipelineCmd(VkPipeline& pipeline);
			void BindDataCmd(VkBuffer vertexBuffer, VkBuffer indexBuffer, VkPipelineLayout& pipelineLayout, std::vector<VkDescriptorSet>& dscrpsets);
			void BindVertexCmd(VkBuffer vertexBuffer);
			void BindIndexCmd(VkBuffer indexBuffer);
			void BindDescriptorSets(VkPipelineLayout& pipelineLayout, std::vector<VkDescriptorSet>& dscrpsets);
			void IndexDrawCmd(int indexCount);
			void DrawCmd(int count);
			void EndRenderPassCmd();
			void EndRecordCmdBuffer();
			void NextSubPass();
			void ResetCmd();
			void WaitReady();
			void Render();
		};
	}
}
#endif // !VKRENDER_H