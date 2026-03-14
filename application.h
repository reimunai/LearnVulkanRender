#pragma once
#include "VulkanBackend.h"

namespace el
{
	namespace app
	{
		using namespace el::backend;
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
			void Render(vector<VkBuffer>& vertexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets);

			void UpdateUniform(vector<void*> uniformBuffersMapped);
			void UpdateUniform(vector<void*> uniformBuffersMapped, glm::vec3 pos);
		};
	}
}