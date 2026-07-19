#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "GltfLoader.h"

#include <iostream>
// 辅助函数：获取访问器数据
template<typename T>
std::vector<T> GetAccessorData(const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    std::vector<T> data;

    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

    size_t stride = accessor.ByteStride(bufferView);
    size_t startOffset = bufferView.byteOffset + accessor.byteOffset;
    size_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    size_t elementSize = componentSize * tinygltf::GetNumComponentsInType(accessor.type);

    data.reserve(accessor.count);

    for (size_t i = 0; i < accessor.count; ++i) {
        size_t offset = startOffset + i * stride;

        //TODO:other componentType
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            T value;
            memcpy(&value, &buffer.data[offset], elementSize);
            data.push_back(value);
        }
        else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            T value;
            memcpy(&value, &buffer.data[offset], elementSize);
            data.push_back(value);
        }
    }

    return data;
}

// 主函数：读取gltf文件并填充顶点数据
bool LoadGLTFMeshVertices(const std::string& filename, std::vector<wl::ElVertexPNT>& vertices, wl::VertexIndice& indices) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // 加载gltf文件
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) {
        std::cout << "警告: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cout << "错误: " << err << std::endl;
    }
    if (!ret) {
        std::cout << "加载gltf文件失败: " << filename << std::endl;
        return false;
    }

    // 清空输出顶点数组
    vertices.clear();

    // 遍历所有网格
    for (const auto& mesh : model.meshes) {
        // 遍历每个基本图元（primitive）
        for (const auto& primitive : mesh.primitives) {
            // 获取位置数据
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> texCoords;

            // 读取位置属性
            auto posIt = primitive.attributes.find("POSITION");
            if (posIt != primitive.attributes.end()) {
                const tinygltf::Accessor& posAccessor = model.accessors[posIt->second];
                positions = GetAccessorData<glm::vec3>(model, posAccessor);
            }

            // 读取法线属性
            auto normalIt = primitive.attributes.find("NORMAL");
            if (normalIt != primitive.attributes.end()) {
                const tinygltf::Accessor& normalAccessor = model.accessors[normalIt->second];
                normals = GetAccessorData<glm::vec3>(model, normalAccessor);
            }

            // 读取UV坐标属性（通常为TEXCOORD_0）
            auto texIt = primitive.attributes.find("TEXCOORD_0");
            if (texIt != primitive.attributes.end()) {
                const tinygltf::Accessor& texAccessor = model.accessors[texIt->second];
                texCoords = GetAccessorData<glm::vec2>(model, texAccessor);
            }

            // 获取索引数据（如果有）
            //std::vector<uint32_t> _indices;
            if (primitive.indices >= 0) {
                const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    auto shortIndices = GetAccessorData<uint16_t>(model, indexAccessor);
                    indices.assign(shortIndices.begin(), shortIndices.end());
                }
                else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    indices = GetAccessorData<uint32_t>(model, indexAccessor);
                }
                else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    auto byteIndices = GetAccessorData<uint8_t>(model, indexAccessor);
                    indices.assign(byteIndices.begin(), byteIndices.end());
                }
            }

                size_t vertexCount = positions.size();
                for (size_t i = 0; i < vertexCount; ++i) {
                    wl::ElVertexPNT vertex;
                    vertex.pos = positions[i];
                    vertex.normal = (i < normals.size()) ? normals[i] : glm::vec3(0.0f);
                    vertex.texCoord = (i < texCoords.size()) ? texCoords[i] : glm::vec2(0.0f);
                    vertices.push_back(vertex);
                }
            //else {
            //    // 使用索引构建顶点数组
            //    for (uint32_t index : indices) {
            //        el::ElVertexPNT vertex;
            //        vertex.pos = positions[index];
            //        vertex.normal = (index < normals.size()) ? normals[index] : glm::vec3(0.0f);
            //        vertex.texCoord = (index < texCoords.size()) ? texCoords[index] : glm::vec2(0.0f);
            //        vertices.push_back(vertex);
            //    }
            //}

            std::cout << "已加载 " << vertices.size() << " 个顶点" << std::endl;
        }
    }

    return true;
}