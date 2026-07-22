#include "Mesh.h"
#include "GltfLoader.h"
#include "vma/vk_mem_alloc.h"
namespace wl::component
{
	Mesh::Mesh(std::string filePath, VmaAllocator vmaAllocator)
	{
		using wl::v1::backend::Buffer;
		LoadGLTFMeshVertices(filePath, vertices, indices);

		vertexBuffer = Buffer::Builder(vmaAllocator, sizeof(wl::ElVertexPNT) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
			.Build();
		vertexBuffer.CopyDataToBuffer(vertices.data());

		indiceBuffer = Buffer::Builder(vmaAllocator, indices.size() * sizeof(indices[0]), VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
			.Build();
		indiceBuffer.CopyDataToBuffer(indices.data());
	}
}
