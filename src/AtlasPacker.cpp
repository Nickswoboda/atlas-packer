#include "AtlasPacker.h"

#include <algorithm>
#include <iostream>
#include <chrono>

ImageData CreateAtlasImageData(const std::vector<ImageData>& images, std::unordered_map<std::string, Vec2> placement, int width, int height)
{
	int channels = 4;
	int atlas_pitch = width * channels;

	unsigned char* pixels = new unsigned char[(size_t)height * atlas_pitch]();

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
	std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();

	Vec2 size = EstimateAtlasSize(image_data);

	//try algo, if can't fit everything, increase size and try again
	std::unordered_map<std::string, Vec2> placement;
	while (placement.empty()) {
		if (size.x > max_width_ || size.y > max_height_) {
			return ImageData();
		}
		std::cout << "Trying: " << size.x << ", " << size.y << "\n";

		placement = PackAtlasRects(image_data, size);

		if (!placement.empty()) break;
		size.x == size.y ? size.x *= 2 : size.y = size.x;
	}

	stats_.atlas_area = size.x * size.y;
	stats_.unused_area = stats_.atlas_area - stats_.total_images_area;
	stats_.packing_efficiency = (stats_.total_images_area / (float)stats_.atlas_area) * 100;

	data_json_ = CreateJsonFile(image_data, placement);
	ImageData atlas_data = CreateAtlasImageData(image_data, placement, size.x, size.y);

	std::chrono::steady_clock::time_point end_time = std::chrono::high_resolution_clock::now();
	stats_.time_elapsed_in_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

	return atlas_data;
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

std::unordered_map<std::string, Vec2> AtlasPacker::PackAtlasRects(std::vector<ImageData>& images, Vec2 size)
{
	std::sort(images.begin(), images.end(), [](ImageData a, ImageData b) {return a.height_ > b.height_; });

	std::unordered_map<std::string, Vec2> placement;

	int pen_x = 0, pen_y = 0;
	int next_pen_y = images[0].height_;

	for (auto& image : images) {

		while (pen_x + image.width_ >= size.x) {
			pen_x = 0;
			pen_y += next_pen_y + pixel_padding_;
			next_pen_y = image.height_;

			//unable to fit everything in atlas
			if (pen_y + image.height_ >= size.y) {
				return std::unordered_map<std::string, Vec2>();
			}
		}

		placement[image.path_name] = { pen_x, pen_y };

		pen_x += image.width_ + pixel_padding_;
	}

	return placement;
}

Vec2 AtlasPacker::EstimateAtlasSize(const std::vector<ImageData>& image_data)
{
	stats_.total_images_area = 0;
	for (const auto& image : image_data) {
		stats_.total_images_area += image.width_ * image.height_;
	}
	Vec2 size{ 16, 16 };
	while (size.x * size.y < stats_.total_images_area) {
		size.x == size.y ? size.x *= 2 : size.y = size.x;
	}
	return size;
}
