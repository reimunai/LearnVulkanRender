#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <fstream>
using  std::string;
using std::vector;
namespace el
{
	namespace resourcemanagement
	{
		class ShaderLoader
		{
		public:
			static VkShaderModule createShaderModule(const vector<char>& code, VkDevice device);
			static std::vector<char> readFile(const string& filename);
		};
	}
}
