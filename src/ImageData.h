#pragma once

#include <vector>
#include <array>
#include <unordered_set>

struct Vec2
{
	int x = 0;
	int y = 0;
};

struct Rect
{
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
};

constexpr int MAX_IMAGES = 512;

struct ImageData
{
	Rect rects_[MAX_IMAGES];
	unsigned char* data_[MAX_IMAGES];
	std::string paths_[MAX_IMAGES];

	int num_images_ = 0;
};

void GetImageData(const std::vector<std::string>& paths, ImageData& image_data);