#include "ImageData.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <iostream>

void GetImageData(const std::vector<std::string>& paths, ImageData& image_data)
{
	image_data.num_images_ = 0;

	for (int i = 0; i < paths.size(); ++i) {

		image_data.data_[i] = stbi_load(paths[i].c_str(), &image_data.rects_[i].w, &image_data.rects_[i].h, nullptr, 4);

		if (image_data.data_[i] == nullptr) {
			std::cout << "unable to load image";
		}

		std::string file_name = std::filesystem::path(paths[i]).generic_u8string();
		image_data.paths_[i] = file_name;

		++image_data.num_images_;
	}

}
