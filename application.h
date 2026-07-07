#pragma once
#ifndef APPLICATION_H
#define APPLICATION_H

#include "VulkanBackend.h"

namespace wl
{
	namespace app
	{
		using namespace wl::backend;
		//TODO:
		//½ö²âÊÔ
		class Application
		{
		public:
			VulkanBackend* backend;
			VulkanPipeline* pipelineManager;
			//pipeline Family
			VkDescriptorSetLayout setLayout;
			VkPipelineLayout layout;
			VkRenderPass rendPass;
			VkPipeline pipeline;
		public:
			VulkanMemory* memory;

		public:
			Application();
			void InitPipelineFamily(PipelineConfig config,vector<VkDescriptorSetLayoutBinding>& binds);
			VkDevice GetDevice();
			VkDescriptorSetLayout GetDescriptorSetLayout();
			void Render(vector<uint16_t>& indexCounts,
				vector<VkBuffer>& vertexBuffers, vector<VkBuffer>& indexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets);

			void UpdateUniform(vector<void*> uniformBuffersMapped);
			void UpdateUniform(vector<void*> uniformBuffersMapped, glm::vec3 pos);
		};
	}
}

#endif // !APPLICATION_H