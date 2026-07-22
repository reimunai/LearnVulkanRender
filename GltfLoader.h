#pragma once
#ifndef GLTFLOADER_H
#define GLTFLOADER_H

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include<tiny_gltf.h>
#include "VertexStruct.h"

template<typename T>
std::vector<T> GetAccessorData(const tinygltf::Model& model, const tinygltf::Accessor& accessor);

bool LoadGLTFMeshVertices(const std::string& filename, std::vector<wl::ElVertexPNT>& vertices, std::vector<wl::VertexIndice>& indices);

#endif // !GLTFLOADER_H
