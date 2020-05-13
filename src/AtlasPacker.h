#pragma once

#include "ImageData.h"

#include <unordered_map>
#include <json.hpp>

struct Vec2
{
	int x = 0;
	int y = 0;
};

class AtlasPacker
{
public:
	ImageData CreateAtlas(std::vector<ImageData>& image_data);
	nlohmann::json CreateJsonFile(const std::vector<ImageData>& images, std::unordered_map<std::string, Vec2> placement);
	std::unordered_map<std::string, Vec2> GetTexturePlacements(const std::vector<ImageData>& images, int width, int height);

	int max_width_ = 4096;
	int max_height_ = 4096;

	int pixel_padding_ = 0;
	bool pow_of_2_ = false;

	nlohmann::json data_json_;
};