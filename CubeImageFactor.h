#pragma once
#ifndef CUBE_IMAGE_FACTOR_H
#define CUBE_IMAGE_FACTOR_H

#include <string>
#include <vector>
#include "Image.h"
#include "VkBackend.h"
namespace wl::resource
{
	class CubeImageFactor
	{
	public:
		CubeImageFactor() = default;
		static wl::backend::Image CreateCubeImage(VkDevice device, VmaAllocator allocator, uint32_t width, uint32_t height);
		static void CopyTextureToCubeImage(wl::backend::Image& cubeImage, std::vector<std::string>& filepaths, VmaAllocator allocator, wl::backend::VkBackend& backend);
	};
}
#endif // !CUBE_IMAGE_FACTOR_H
