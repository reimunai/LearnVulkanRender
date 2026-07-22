#pragma once
#ifndef PIPELINE_H
#define PIPELINE_H
#include <memory>

#include "PipelineBuilder.h"
namespace wl::resource
{
	class Pipeline
	{
	public:
		VkDevice device;
		VkPipeline vkpipeline;
		std::shared_ptr<VkRenderPass> renderPass_sptr;
		std::shared_ptr<VkPipelineLayout> pipelineLayout_sptr;
		std::shared_ptr<std::vector<VkFramebuffer>> framebuffers_sptr;

		Pipeline() = default;
		Pipeline(Pipeline&&) = default;
		Pipeline(Pipeline&) = delete;

		Pipeline& operator= (Pipeline&&) = default;
		Pipeline& operator= (Pipeline&) = delete;

		~Pipeline()
		{
			vkDestroyPipeline(device, vkpipeline, nullptr);
		}
	};
}
#endif // !PIPELINE_H