#pragma once
#include "application.h"
using namespace el::app;

Application::Application()
{
	backend = new VulkanBackend();
	backend->Init();
	memory = new VulkanMemory(*backend);
	pipelineManager = new VulkanPipeline(*backend);
}

void Application::InitPipelineFamily(PipelineConfig config,vector<VkDescriptorSetLayoutBinding>& binds)
{
	setLayout = pipelineManager->CreateDescriptorSetLayout(binds);
	layout = pipelineManager->CreatePipelineLayout(setLayout);
	rendPass = backend->GetPresentRenderPass();
	pipeline = pipelineManager->CreatePipeline(config, layout, rendPass);
}

void Application::Render(vector<VkBuffer>& vertexBuffers, vector<vector<VkDescriptorSet>>& descriptorSets)
{
	backend->Render(rendPass, pipeline, layout, vertexBuffers, descriptorSets);
}

VkDevice Application::GetDevice()
{
	return backend->GetDevive();
}

VkDescriptorSetLayout Application::GetDescriptorSetLayout()
{
	return setLayout;
}

void Application::UpdateUniform(vector<void*> uniformBuffersMapped)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.model = modelMat4;
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), backend->surfaceCaps.currentExtent.width / (float)backend->surfaceCaps.currentExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffersMapped[backend->currentFrame], &ubo, sizeof(ubo));
}

void Application::UpdateUniform(vector<void*> uniformBuffersMapped, glm::vec3 pos)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.model = glm::translate(ubo.model, pos);
	//ubo.model = modelMat4;
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), backend->surfaceCaps.currentExtent.width / (float)backend->surfaceCaps.currentExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffersMapped[backend->currentFrame], &ubo, sizeof(ubo));
}