#pragma once

#include <vector>
#include <array>
#include <unordered_set>

struct Vec2
{
	int x = 0;
	int y = 0;
};

constexpr int MAX_IMAGES = 512;
struct ImageData
{
	Vec2 sizes_[MAX_IMAGES];
	Vec2 pos_[MAX_IMAGES];
	unsigned char* data_[MAX_IMAGES];
	std::string paths_[MAX_IMAGES];

	int num_images_ = 0;
};

void GetImageData(const std::vector<std::string>& paths, ImageData& image_data);