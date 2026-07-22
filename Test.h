#ifndef FREE_TEST
#define FREE_TEST
#include "vulkan/vulkan.h"

#include "VkBackend.h"
namespace test
{
	void Test_Run();
	VkPipeline CreateSkyBoxPipeline(wl::backend::VkBackend* backend, VkPipelineLayout& pipelineLayout, VkRenderPass& presentRenderPass);
	VkPipeline CreateLambertPipeline(wl::backend::VkBackend* backend, VkPipelineLayout& pipelineLayout, VkRenderPass& presentRenderPass);
}
#endif // !FREE_TEST
