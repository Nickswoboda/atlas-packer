#pragma once

#include <vector>
#include <unordered_set>

struct ImageData
{
	int width_;
	int height_;
	unsigned char* data_ = nullptr;
};

std::vector<ImageData> GetImageData(const std::unordered_set<std::string>& paths);