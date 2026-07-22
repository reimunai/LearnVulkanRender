#pragma once
#ifndef VKRESOURSES_H
#define VKRESOURSES_H
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

#include "VkBackend.h"
#include "VertexStruct.h"

namespace wl
{
	namespace backend
	{
		class VkResources
		{
		public:
			VmaAllocator vmaAllocator;
		private:
			VkBackend* vkBackend;

			VkDescriptorPool descriptorPool;

		public:
			VkResources(VkBackend& vkBackend);
			~VkResources();

			//memory
			ElBuffer CreateTransferBuffer(void* data, VkDeviceSize size);
			void CreateIndexBuffer(std::vector<wl::VertexIndice>& indices, ElBuffer& elBuffer);
			void CreateVertexBuffer(std::vector<ElVertexPNT>& vertices, ElBuffer& elBuffer);
			void CreateUniformBuffer(VkDeviceSize bufferSize, std::vector<ElBuffer>& elUniformBuffers);
			void CreateImage(int texWidth, int texHeight, VkImage& image, VmaAllocation& allocation);
			VkDescriptorSetLayout CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings);
			VkRenderPass CreateRenderPass(VkFormat imageFormat);
			VkPipeline CreatePipeline(PipelineConfig config, VkPipelineLayout& pipelineLayout, VkRenderPass& renderPass);
			std::vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout);
			VkImageView CreateTextureImageView(VkImage image);


			VkPipelineLayout CreatePipelineLayout(VkDescriptorSetLayout& descriptorSetLayout);
			VkSampler CreateTextureSampler();

			void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
			void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		private:
			void InitVma();
			void InitDescriptorPool();
		};
	}
}
#endif // !VKRESOURSES_H
