#pragma once
#ifndef MESH_H
#define MESH_H
#include <vector>
#include <string>
#include "VertexStruct.h"

#include "Buffer.h"
namespace wl::component
{
	class Mesh
	{
	public:
		std::vector<wl::ElVertexPNT> vertices;
		std::vector<wl::VertexIndice> indices;

		wl::v1::backend::Buffer vertexBuffer;
		wl::v1::backend::Buffer indiceBuffer;

		Mesh(std::string filePath, VmaAllocator vmaAllocator);
	};
}
#endif // !MESH_H
