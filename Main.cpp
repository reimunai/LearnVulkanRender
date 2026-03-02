#include "VulkanBackend.h"
#include "Component.hpp"
#include "shaderLoader.h"
#include <string>


using namespace myapplication::backend;
using namespace myapplication::component;
using namespace myapplication::resourcemanagement;
struct vvertex
{
	glm::vec2 pos;
	glm::vec3 color;
};
//int main()
//{
//	Uint64 startTime = SDL_GetPerformanceCounter();
//	Uint64 frequency = SDL_GetPerformanceFrequency();
//
//	VulkanBackend* engine = new VulkanBackend();
//	engine->init();
//
//	bool quit = false;
//	SDL_Event event;
//	while (!quit)
//	{
//		while (SDL_PollEvent(&event))
//		{
//			if (event.type == SDL_EVENT_QUIT)
//			{
//				quit = true;
//			}
//		}
//
//		engine->render();
//
//		Uint64 currentTime = SDL_GetPerformanceCounter();
//		float deltaTime = (currentTime - startTime) / (float)frequency;
//		startTime = currentTime;
//
//		// ¼ÆËã FPS
//		float fps = 1.0f / deltaTime;
//		std::string titlie = std::to_string(fps);
//		SDL_SetWindowTitle(engine->window, titlie.c_str());
//	}
//
//	SDL_Quit();
//}

int main()
{
	Uint64 startTime = SDL_GetPerformanceCounter();
	Uint64 frequency = SDL_GetPerformanceFrequency();

	VulkanBackend* engine = new VulkanBackend();
	engine->init();
	VulkanMemory* vulkanMemory = new VulkanMemory(*engine);
	VulkanPipeline* vulkanPipeline = new VulkanPipeline(*engine);
	auto vertShaderCode = ShaderLoader::readFile("shaders/vert.spv");
	auto fragShaderCode = ShaderLoader::readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = ShaderLoader::createShaderModule(vertShaderCode, engine->device);
	VkShaderModule fragShaderModule = ShaderLoader::createShaderModule(fragShaderCode, engine->device);

	PipelineConfig config = {
		.shaderID = 0,
		.cullMode = myapplication::CullMode::None,
		.blendMode = myapplication::BlendMode::None,
		.vertexShaderModule = vertShaderModule,
		.fragShaderModule = fragShaderModule,
	};

	vector<VkDescriptorSetLayoutBinding> binds = {

		{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		//.pImmutableSamplers = nullptr  Optional
		}
	};

	VkDescriptorSetLayout setLatout = vulkanPipeline->CreateDescriptorSetLayout(binds);
	VkPipelineLayout layout = vulkanPipeline->CreatePipelineLayout(setLatout);
	VkRenderPass renderpass = vulkanPipeline->CreateRenderPass();
	VkPipeline pipeline = vulkanPipeline->CreatePipeline(config, layout, renderpass);

	engine->renderPass = renderpass;
	engine->descriptorSetLayout = setLatout;
	engine->pipelineLayout = layout;
	engine->pipeline = pipeline;
	
	auto descriptorSets = vulkanMemory->CreateDescriptorSets(engine->descriptorSetLayout, engine->uniformBuffers);
	engine->descriptorSets = descriptorSets;

	Entity e1;
	auto renderer1 = e1.AddComponent<MeshRenderer>();
	renderer1->mesh = {
		{{-0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{-0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},

		{{-0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},

		{{-0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{-0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},

		{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{-0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},

	};
	renderer1->RenderInit(*vulkanMemory);

	vector<VkBuffer> buffers(0);
	buffers.push_back(renderer1->buffer);

	bool quit = false;
	SDL_Event event;
	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				quit = true;
			}
		}

		engine->render1(buffers);

		Uint64 currentTime = SDL_GetPerformanceCounter();
		float deltaTime = (currentTime - startTime) / (float)frequency;
		startTime = currentTime;

		// ¼ÆËã FPS
		float fps = 1.0f / deltaTime;
		std::string titlie = std::to_string(fps);
		SDL_SetWindowTitle(engine->window, titlie.c_str());
	}

	SDL_Quit();
}