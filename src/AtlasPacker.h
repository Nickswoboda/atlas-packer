#pragma once

#include "ImageData.h"

#include <unordered_map>

struct Stats
{
	double time_elapsed_in_ms = 0.0;
	int total_images_area = 0;
	int atlas_area = 0;
	int unused_area = 0;
	float packing_efficiency = 0.0f;

};

class AtlasPacker
{
public:
	int CreateAtlas(ImageData& image_data);
	std::string GetAtlasData(const ImageData& images);
	bool PackAtlasRects(ImageData& images, Vec2 size);
	Vec2 EstimateAtlasSize(const ImageData& images);
	
	int max_width_ = 4096;
	int max_height_ = 4096;
	bool fixed_size = false;
	bool force_square = false;

	int pixel_padding_ = 0;
	bool pow_of_2_ = false;
	
	Vec2 size_;
	std::string save_data_;
	Stats stats_;
};