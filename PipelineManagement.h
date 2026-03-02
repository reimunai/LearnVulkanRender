#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "VulkanBackend.h"
#include <iostream>
namespace myapplication
{
	namespace resourcemanagement
	{
		using std::vector;
		using namespace myapplication::backend;
		class PipelineStateManagement
		{
			//pilelineLineLayout
			//renderpass
			//descriptorSetLayout
			//pipeline
		private:
			vector<VkDescriptorSetLayout> _descripSetLayouts;
			vector<VkPipelineLayout> _pipelineLayouts;
			vector<VkPipeline> _pipelines;
			
		public:
			VkPipeline CreatePipeline(const PipelineConfig& config, VulkanBackend* backend)
			{
				
			}
			VkPipeline GetPipeline(uint32_t pipelineIndex)
			{
				if (pipelineIndex >= 0 && pipelineIndex < _pipelines.size())
				{
					return _pipelines[pipelineIndex];
				}

				std::cerr << "pipeline not found!" << std::endl;
				exit(0);
			}
		};
	}
}