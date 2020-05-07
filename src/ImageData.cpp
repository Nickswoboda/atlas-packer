#include "ImageData.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <iostream>

std::vector<ImageData> GetImageData(const std::unordered_set<std::string>& paths)
{
	std::vector<ImageData> image_data;

	for (auto& path : paths) {
		int width;
		int height;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, nullptr, 4);

		if (data == nullptr) {
			std::cout << "unable to load image";
		}
		
		std::string file_name = std::filesystem::path(path).stem().u8string();
		image_data.emplace_back(ImageData({file_name, width, height, data }));
	}

	return image_data;
}
