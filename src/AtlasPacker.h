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

enum class Algorithm 
{
	Shelf,
	MaxRects
};

enum class AtlasSizeSolver
{
	Fast,
	BestFit
};
class AtlasPacker
{
public:
	int CreateAtlas(ImageData& image_data);
	std::string GetAtlasMetadata(const ImageData& images);

	bool PackAtlas(ImageData& images, Vec2 size);
	bool PackAtlasShelf(ImageData& images, Vec2 size);
	bool PackAtlasMaxRects(ImageData& images, Vec2 size);

	void GetPossibleContainers(const ImageData& images, std::vector<Vec2>& possible_sizes);
	Vec2 EstimateAtlasSize(const ImageData& images);
	
	int max_width_ = 4096;
	int max_height_ = 4096;
	bool fixed_size_ = false;
	bool force_square_ = false;

	int pixel_padding_ = 0;
	bool pow_of_2_ = false;
	
	Algorithm algo_ = Algorithm::Shelf;
	AtlasSizeSolver size_solver_ = AtlasSizeSolver::Fast;
	std::vector<Vec2> possible_sizes_;
	Vec2 size_;
	std::string metadata_;
	Stats stats_;
};