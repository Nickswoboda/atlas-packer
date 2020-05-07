#pragma once

#include <vector>
#include <unordered_set>

struct ImageData
{
	int width_ = 0;
	int height_ = 0;
	unsigned char* data_ = nullptr;
};

std::vector<ImageData> GetImageData(const std::unordered_set<std::string>& paths);