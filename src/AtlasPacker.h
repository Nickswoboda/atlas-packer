#pragma once

#include "ImageData.h"

#include <unordered_map>

constexpr int MAX_DIMENSIONS = 4096;
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

enum class SizeSolver
{
	Fixed,
	Fast,
	BestFit
};
class AtlasPacker
{
public:
	int CreateAtlas(ImageData& image_data);
	std::string GetAtlasMetadata(const ImageData& images);

	void WriteAtlasImageData(ImageData& images, int width, int height);
	bool PackAtlas(ImageData& images, Vec2 size);
	bool PackAtlasShelf(ImageData& images, Vec2 size);
	bool PackAtlasMaxRects(ImageData& images, Vec2 size);
	bool IntersectsRect(const Rect& new_rect, const Rect& free_rect);

	void GetPossibleContainers(const ImageData& images, std::vector<Vec2>& possible_sizes);
	void PushSplitRects(std::vector<Rect>& rects, const Rect& new_rect, Rect free_rect);

	bool EnclosedInRect(const Rect& a, const Rect& b);
	
	int max_width_ = MAX_DIMENSIONS;
	int max_height_ = MAX_DIMENSIONS;
	bool force_square_ = false;

	int pixel_padding_ = 0;
	bool pow_of_2_ = false;
	
	Algorithm algo_ = Algorithm::Shelf;
	SizeSolver size_solver_ = SizeSolver::Fast;
	std::vector<Vec2> possible_sizes_;
	Vec2 size_;
	std::string metadata_;
	Stats stats_;
};