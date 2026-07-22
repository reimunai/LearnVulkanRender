#pragma once
#ifndef SAMPLER_BUILDER_H
#define SAMPLER_BUILDER_H
#include <iostream>

#include "VkBackend.h"
namespace wl::backend
{
	class SamplerBuilder
	{
		VkBackend& backend;
		VkSamplerCreateInfo samplerInfo;
	public:
		SamplerBuilder(VkBackend& backend);

		SamplerBuilder& SetAddressMode(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w)
		{
			samplerInfo.addressModeU = u;
			samplerInfo.addressModeV = v;
			samplerInfo.addressModeW = w;

			return *this;
		}
		VkSampler Builder()
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(backend.phyDevice, &properties);
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
			VkSampler s;
			if (vkCreateSampler(backend.device, &samplerInfo, nullptr, &s) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture sampler!");
			}
			return s;
		}
	};
}
#endif // SAMPLER_BUILDER_H
