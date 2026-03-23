#ifndef COMPONENT_H


#pragma once
#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "VulkanBackend.h"
namespace el
{
	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;
		//glm::vec3 normal;
	};
	namespace component
	{
		using namespace el::backend;
		class IComponent
		{
		public:
			virtual ~IComponent() = default;
			virtual void Update(){};
		};

		class Entity
		{
		private:
			std::vector<std::shared_ptr<IComponent>> components;

		public:
			template<typename T>
			T* AddComponent()
			{
				auto pComponent = std::make_shared<T>();
				components.push_back(pComponent);
				return pComponent.get();
			}

			void UpdateAll()
			{
				for (auto& component : components)
				{
					component->Update();
				}
			}
		};

		class MeshRenderer : public IComponent
		{
		public:
			std::vector<ElVertexPNT> mesh;
			el::VertexIndice indices;

			VkBuffer vertexBuffer;
			VmaAllocation allocation;
			VmaAllocationInfo allocationInfo;

			VkBuffer indexBuffer;
			VmaAllocation indexAllocation;
			VmaAllocationInfo indexAllocationInfo;
			// Í¨¹ý IComponent ¼Ì³Ð
			void RenderInit(VulkanMemory& vulkanMemory)
			{
				if (mesh.size() != 0)
				{
					vulkanMemory.CreateVertexBuffer(mesh, vertexBuffer, allocation, allocationInfo);
					vulkanMemory.CreateIndexBuffer(indices, indexBuffer, indexAllocation, indexAllocationInfo);
				}
			}

			//vertex & uniform buffer init

			//record bindbuffer & draw cmd
		};

		//TODO:uniform BUffer
		class Transform : public IComponent
		{
		public:
			glm::vec3 position;
			glm::vec3 scale;
			glm::vec3 rotate;

			glm::mat4 GetModelMat()
			{
				glm::mat res = glm::mat4(1.0f);
				
				res = glm::scale(res, scale);
				res = glm::rotate(res, glm::radians(rotate.x), glm::vec3(1.0f, 0.0f, 0.0f));
				res = glm::rotate(res, glm::radians(rotate.y), glm::vec3(0.0f, 1.0f, 0.0f));
				res = glm::rotate(res, glm::radians(rotate.z), glm::vec3(0.0f, 0.0f, 1.0f));

				res = glm::translate(res, position);

				return res;
			}
		};
	}
}
#endif // !COMPONENT_H