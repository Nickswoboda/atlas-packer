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
	std::array<Vec2, MAX_IMAGES> sizes_;
	std::array<Vec2, MAX_IMAGES> pos_;
	std::array<unsigned char*, MAX_IMAGES> data_;
	std::array<std::string, MAX_IMAGES> paths_;

	int num_images_ = 0;
};

void GetImageData(const std::vector<std::string>& paths, ImageData& image_data);