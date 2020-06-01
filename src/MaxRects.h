#pragma once

#include "ImageData.h"

class MaxRects
{
public:
	static bool PackAtlas(ImageData& images, Vec2 size, std::vector<int>& sorted_indices, int padding);
private:
	static bool IntersectsRect(const Rect& new_rect, const Rect& free_rect);
	static void PushSplitRects(const Rect& new_rect, const Rect& free_rect);
	static bool EnclosedInRect(const Rect& a, const Rect& b);
};