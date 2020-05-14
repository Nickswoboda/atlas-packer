#include "ImageData.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <iostream>

void GetImageData(const std::vector<std::string>& paths, ImageData& image_data)
{

	image_data.num_images_ = 0;

	for (int i = 0; i < paths.size(); ++i) {
		int width;
		int height;
		unsigned char* data = stbi_load(paths[i].c_str(), &width, &height, nullptr, 4);

		if (data == nullptr) {
			std::cout << "unable to load image";
		}

		std::string file_name = std::filesystem::path(paths[i]).generic_u8string();
		
		image_data.path_name_[i] = file_name;
		image_data.size_[i].x = width;
		image_data.size_[i].y = height;
		image_data.data_[i] = data;

		++image_data.num_images_;
	}
}
