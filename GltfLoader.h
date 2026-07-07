#ifndef GLTFLOADER_H
#define GLTFLOADER_H
#pragma once


#include<tiny_gltf.h>
#include "VertexStruct.h"

template<typename T>
std::vector<T> GetAccessorData(const tinygltf::Model& model, const tinygltf::Accessor& accessor);

bool LoadGLTFMeshVertices(const std::string& filename, std::vector<wl::ElVertexPNT>& vertices, wl::VertexIndice& indices);

#endif // !GLTFLOADER_H
