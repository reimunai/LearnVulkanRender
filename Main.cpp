#include "VulkanBackend.h"
#include "application.h"
#include "Component.hpp"
#include "shaderLoader.h"
#include <string>

using namespace myapplication::backend;
using namespace myapplication::component;
using namespace myapplication::resourcemanagement;
using namespace myapplication::app;
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

	auto app = new Application();

	auto vertShaderCode = ShaderLoader::readFile("shaders/vert.spv");
	auto fragShaderCode = ShaderLoader::readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = ShaderLoader::createShaderModule(vertShaderCode, app->GetDevice());
	VkShaderModule fragShaderModule = ShaderLoader::createShaderModule(fragShaderCode, app->GetDevice());

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
	app->InitPipelineFamily(config, binds);

	vector<VkBuffer> uniformBuffers;
	vector<VkDeviceMemory> uniformMemorys;
	vector<VkDescriptorSet> dscrptSets;
	vector<void*> map;
	auto size = sizeof(myapplication::UniformBufferObject);
	app->memory->CreateUniformBuffer(size, uniformBuffers, uniformMemorys, map);
	dscrptSets = app->memory->CreateDescriptorSets(app->GetDescriptorSetLayout(), uniformBuffers);
	vector<vector<VkDescriptorSet>> setSets(0);
	setSets.push_back(dscrptSets);
	setSets.push_back(dscrptSets);
	setSets.push_back(dscrptSets);


	Entity e1;
	auto renderer1 = e1.AddComponent<MeshRenderer>();
	renderer1->mesh = {
		{{-0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{-0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},

		{{-0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, 0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},

		//{{-0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		//{{-0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		//{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},

		//{{0.5f, -0.5f, 0.0f}, {0.6f, 0.6f, 0.6f}},
		//{{-0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		//{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},

	};
	renderer1->RenderInit(*(app->memory));

	Entity e2;
	auto renderer2 = e2.AddComponent<MeshRenderer>();
	renderer2->mesh = {
		{{-0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		{{-0.5f, 0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},

		{{-0.5f, 0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, 0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},

		//{{-0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		//{{-0.5f, -0.5f, 2.0f}, {0.6f, 0.6f, 0.6f}},
		//{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},

		//{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		//{{-0.5f, -0.5f, 2.0f}, {0.6f, 0.6f, 0.6f}},
		//{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},

	};
	renderer2->RenderInit(*(app->memory));

	Entity e3;
	auto renderer3 = e3.AddComponent<MeshRenderer>();
	renderer3->mesh = {
		{{-0.5f, -0.5f, -1.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, -1.0f}, {0.6f, 0.6f, 0.6f}},
		{{-0.5f, 0.5f, -1.0f}, {0.6f, 0.6f, 0.6f}},

		{{-0.5f, 0.5f, -1.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, -0.5f, -1.0f}, {0.6f, 0.6f, 0.6f}},
		{{0.5f, 0.5f, -1.0f}, {0.6f, 0.6f, 0.6f}},

		//{{-0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		//{{-0.5f, -0.5f, 2.0f}, {0.6f, 0.6f, 0.6f}},
		//{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},

		//{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},
		//{{-0.5f, -0.5f, 2.0f}, {0.6f, 0.6f, 0.6f}},
		//{{0.5f, -0.5f, 1.0f}, {0.6f, 0.6f, 0.6f}},

	};
	renderer3->RenderInit(*(app->memory));

	vector<VkBuffer> buffers(0);
	buffers.push_back(renderer1->buffer);
	buffers.push_back(renderer2->buffer);
	buffers.push_back(renderer3->buffer);

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

		Uint64 currentTime = SDL_GetPerformanceCounter();
		float deltaTime = (currentTime - startTime) / (float)frequency;
		startTime = currentTime;

		app->UpdateUniform(map, glm::vec3(1.0f, 0.0f, 0.0f));
		app->Render(buffers, setSets);

		// ¼ÆËã FPS
		float fps = 1.0f / deltaTime;
		std::string titlie = std::to_string(fps);
		//SDL_SetWindowTitle(engine->window, titlie.c_str());
	}

	SDL_Quit();
}