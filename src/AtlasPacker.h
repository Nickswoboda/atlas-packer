#pragma once

#include "ImageData.h"

#include <unordered_map>
#include <json.hpp>

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
	nlohmann::json CreateJsonFile(const ImageData& images);
	bool PackAtlasRects(ImageData& images, Vec2 size);
	Vec2 EstimateAtlasSize(const ImageData& images);

	int max_width_ = 4096;
	int max_height_ = 4096;

	int pixel_padding_ = 0;
	bool pow_of_2_ = false;

	nlohmann::json data_json_;
	
	Stats stats_;
};