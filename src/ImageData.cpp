#include "ImageData.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <iostream>

void GetImageData(const std::vector<std::string>& paths, ImageData& image_data)
{

	//clear previous image data
	for (int i = 0; i < image_data.num_images_; ++i) {
		if (image_data.data_[i] != nullptr) {
			stbi_image_free(image_data.data_[i]);
		}
	}

	image_data.num_images_ = 0;

	for (int i = 0; i < paths.size(); ++i) {

		image_data.data_[i] = stbi_load(paths[i].c_str(), &image_data.rects_[i].w, &image_data.rects_[i].h, nullptr, 4);

		if (image_data.data_[i] == nullptr) {
			std::cout << "Unable to load " << paths[i] << ".\n";
		}

		image_data.paths_[i] = std::filesystem::path(paths[i]).generic_u8string();;

		++image_data.num_images_;
	}

}
