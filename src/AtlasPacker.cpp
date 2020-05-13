#include "AtlasPacker.h"

#include <algorithm>
#include <iostream>

ImageData CombineAtlas(const std::vector<ImageData>& images, std::unordered_map<std::string, Vec2> placement, int width, int height)
{
	int channels = 4;
	int atlas_pitch = width * channels;

	unsigned char* pixels = new unsigned char[(size_t)height * atlas_pitch]();

	nlohmann::json json;
	for (auto& image : images) {

		int image_pitch = image.width_ * channels;

		int pen_x = placement[image.path_name].x * channels;
		int pen_y = placement[image.path_name].y;

		for (int row = 0; row < image.height_; ++row) {
			for (int col = 0; col < image_pitch; ++col) {
				int x = pen_x + col;
				int y = pen_y + row;
				pixels[y * (atlas_pitch)+x] = image.data_[row * (image_pitch)+col];
			}
		}
	}

	return ImageData{ "atlas", width, height, pixels };
}

ImageData AtlasPacker::CreateAtlas(std::vector<ImageData>& image_data)
{
	std::sort(image_data.begin(), image_data.end(), [](ImageData a, ImageData b) {return a.height_ > b.height_; });

	int total_area = 0;
	for (const auto& image : image_data) {
		total_area += image.width_ * image.height_;
	}

	int atlas_width = 16;
	int atlas_height = 16;
	while (atlas_width * atlas_height < total_area) {
		atlas_width == atlas_height ? atlas_width *= 2 : atlas_height = atlas_width;
	}

	std::unordered_map<std::string, Vec2> placement;
	while (placement.empty()) {
		if (atlas_width > max_width_ || atlas_height > max_height_) {
			return ImageData();
		}
		std::cout << "Trying: " << atlas_width << ", " << atlas_height << "\n";

		placement = GetTexturePlacements(image_data, atlas_width, atlas_height);

		if (!placement.empty()) break;
		atlas_width == atlas_height ? atlas_width *= 2 : atlas_height = atlas_width;
	}

	data_json_ = CreateJsonFile(image_data, placement);
	return CombineAtlas(image_data, placement, atlas_width, atlas_height);
}

nlohmann::json AtlasPacker::CreateJsonFile(const std::vector<ImageData>& images, std::unordered_map<std::string, Vec2> placement)
{
	nlohmann::json json;

	for (const auto& image : images) {
		json[image.path_name]["x_pos"] = placement[image.path_name].x;
		json[image.path_name]["y_pos"] = placement[image.path_name].y;
		json[image.path_name]["width"] = image.width_;
		json[image.path_name]["height"] = image.height_;
	}

	return json;
}

std::unordered_map<std::string, Vec2> AtlasPacker::GetTexturePlacements(const std::vector<ImageData>& images, int width, int height)
{
	std::unordered_map<std::string, Vec2> placement;

	int pen_x = 0, pen_y = 0;
	int next_pen_y = images[0].height_;

	for (auto& image : images) {

		while (pen_x + image.width_ >= width) {
			pen_x = 0;
			pen_y += next_pen_y + pixel_padding_;
			next_pen_y = image.height_;

			if (pen_y + image.height_ >= height) {
				return std::unordered_map<std::string, Vec2>();
			}
		}

		placement[image.path_name] = { pen_x, pen_y };

		pen_x += image.width_ + pixel_padding_;
	}

	return placement;
}