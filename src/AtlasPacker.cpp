#include "AtlasPacker.h"

#include <algorithm>
#include <iostream>
#include <chrono>

void CreateAtlasImageData(ImageData& images, int width, int height)
{
	int channels = 4;
	int atlas_pitch = width * channels;

	unsigned char* pixels = new unsigned char[(size_t)height * atlas_pitch]();

	for (int i = 0; i < images.num_images_; ++i) {

		int image_pitch = images.size_[i].x * channels;

		int pen_x = images.pos_[i].x * channels;
		int pen_y = images.pos_[i].y;

		for (int row = 0; row < images.size_[i].y; ++row) {
			for (int col = 0; col < image_pitch; ++col) {
				int x = pen_x + col;
				int y = pen_y + row;
				pixels[y * (atlas_pitch)+x] = images.data_[i][row * (image_pitch)+col];
			}
		}
	}

	images.atlas_index_ = images.num_images_;
	images.size_[images.atlas_index_] = { width, height };
	images.data_[images.atlas_index_] = pixels;
}

void AtlasPacker::CreateAtlas(ImageData& image_data)
{
	std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();

	Vec2 size = EstimateAtlasSize(image_data);

	//try algo, if can't fit everything, increase size and try again
	std::unordered_map<std::string, Vec2> placement;
	while (!PackAtlasRects(image_data, size)) {
		if (size.x > max_width_ || size.y > max_height_) {
			return;
		}

		if (!placement.empty()) break;
		size.x == size.y ? size.x *= 2 : size.y = size.x;
	}

	stats_.atlas_area = size.x * size.y;
	stats_.unused_area = stats_.atlas_area - stats_.total_images_area;
	stats_.packing_efficiency = (stats_.total_images_area / (float)stats_.atlas_area) * 100;

	data_json_ = CreateJsonFile(image_data, placement);
	CreateAtlasImageData(image_data, size.x, size.y);

	std::chrono::steady_clock::time_point end_time = std::chrono::high_resolution_clock::now();
	stats_.time_elapsed_in_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
}

nlohmann::json AtlasPacker::CreateJsonFile(const ImageData& images, std::unordered_map<std::string, Vec2> placement)
{
	nlohmann::json json;

	for (int i = 0; i < images.num_images_; ++i) {
		json[images.path_name_[i]]["x_pos"] = images.path_name_[i];
		json[images.path_name_[i]]["y_pos"] = images.path_name_[i];
		json[images.path_name_[i]]["width"] = images.size_[i].x;
		json[images.path_name_[i]]["height"] = images.size_[i].y;
	}

	return json;
}

bool AtlasPacker::PackAtlasRects(ImageData& images, Vec2 size)
{

	//std::sort(images.begin(), images.end(), [](ImageData a, ImageData b) {return a.height_ > b.height_; });

	std::unordered_map<std::string, Vec2> placement;

	int pen_x = 0, pen_y = 0;
	int next_pen_y = images.size_[0].y;

	for (int i = 0; i < images.num_images_; ++i) {

		while (pen_x + images.size_[i].x >= size.x) {
			pen_x = 0;
			pen_y += next_pen_y + pixel_padding_;
			next_pen_y = images.size_[i].y;

			//unable to fit everything in atlas
			if (pen_y + images.size_[i].y >= size.y) {
				return false;
			}
		}

		images.pos_[i] = { pen_x, pen_y };

		pen_x += images.size_[i].x + pixel_padding_;
	}

	return true;
}

Vec2 AtlasPacker::EstimateAtlasSize(const ImageData& images)
{
	stats_.total_images_area = 0;

	for (int i = 0; i < images.num_images_; ++i){
		stats_.total_images_area += images.size_[i].x * images.size_[i].y;
	}
	Vec2 size{ 16, 16 };
	while (size.x * size.y < stats_.total_images_area) {
		size.x == size.y ? size.x *= 2 : size.y = size.x;
	}
	return size;
}
