#pragma once
#ifndef RENDERER_H
#define RENDERER_H
#include <memory>
#include <vector>
#include "VkRender.h"
#include "Mesh.h"
namespace wl::component
{
	class Renderer
	{
		public:
			std::shared_ptr<Mesh> mesh_sptr;
			std::shared_ptr<VkRenderPass> renderPass_sptr;
			std::shared_ptr<VkPipeline> pipeline_sptr;
			std::shared_ptr<VkPipelineLayout> pipelineLayout_sptr;
			std::shared_ptr <std::vector<VkFramebuffer>> frameBuffers_sptr;
			std::shared_ptr<std::vector<VkDescriptorSet>> dscrptorSets_sptr;

			std::vector<VkClearValue> clearValues;
			
			void recordRenderCmd(wl::backend::VkRenderPresent&);

			Renderer() = default;
			Renderer& operator=(const Renderer&) = default;
		
		
	};
}
#endif // !RENDERER_H
