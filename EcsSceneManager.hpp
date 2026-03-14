#pragma once
#include <cstddef>
#include <vector>
#include <map>
#include <memory>
#include <bitset>

namespace el
{
	namespace ecsstylecomponent
	{
		const int MAX_COMPONENTS = 32;
		const int MAX_ENTITIES = 100;

		using ComponentId = std::size_t;
		using EntityId = std::size_t;
		using std::vector;
		using std::map;

		template <typename T>
		ComponentId getComponentTypeId()
		{
			static ComponentId id = 0;
			return id++;
		}

		class IComponent
		{
		public:
			virtual ~IComponent() = default;
		};

		struct Transform : public IComponent
		{
			float position[3];
			float rotate[3];
			float scale[3];
		};

		class worldManager
		{
		private:
			EntityId nextId = 0;
			vector<map<EntityId, std::unique_ptr<IComponent>>> componentArrays;
			map<EntityId, std::bitset<MAX_COMPONENTS>> entitySignatures;
		public:
			EntityId createEntity()
			{
				EntityId id = nextId++;
				entitySignatures[id] = std::bitset<MAX_COMPONENTS>();
			}

			template<typename T>
			T* getComponent(EntityId entity)
			{
				ComponentId cid = getComponentTypeId<T>();
				auto it = componentArrays[cid].find(entity);
				if (it != componentArrays[cid].end())
				{
					return static_cast<T*>(it->second.get());
				}

				return nullptr;
			}
		};

	}
}