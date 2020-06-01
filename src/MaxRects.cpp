#include "MaxRects.h"

static int pixel_padding_ = 0;
static std::vector<Rect> free_rects_;

bool MaxRects::PackAtlas(ImageData& images, Vec2 size, std::vector<int>& sorted_indices, int padding)
{
	pixel_padding_ = padding;

	//start with whole atlas being available
	free_rects_.clear();
	free_rects_.push_back({ 0,0, size.x, size.y });

	for (int image = 0; image < images.num_images_; ++image) {

		if (free_rects_.empty()) {
			return false;
		}

		int curr_idx = sorted_indices[image];

		int best_short_side_fit = 4096;
		int best_fit_index = 0;
		for (int i = 0; i < free_rects_.size(); ++i) {
			int leftover_width = free_rects_[i].w - images.rects_[curr_idx].w;
			int leftover_height = free_rects_[i].h - images.rects_[curr_idx].h;
			int shortest_side = std::min(leftover_width, leftover_height);

			//if shortest side < 0 then image did not fit into free rect
			if (shortest_side >= 0 && shortest_side < best_short_side_fit) {
				best_short_side_fit = shortest_side;
				best_fit_index = i;
			}
		}

		//didnt find any fits
		if (best_short_side_fit == 4096) {
			return false;
		}

		images.rects_[curr_idx].x = free_rects_[best_fit_index].x;
		images.rects_[curr_idx].y = free_rects_[best_fit_index].y;

		//used to not waste time going over the new split rects that are added
		int num_rects_left = free_rects_.size();
		for (int i = 0; i < num_rects_left; ++i) {
			if (IntersectsRect(images.rects_[curr_idx], free_rects_[i])) {
				//split intersected free rects into at most 4 new smaller rects
				PushSplitRects(images.rects_[curr_idx], free_rects_[i]);

				free_rects_.erase(free_rects_.begin() + i);
				--i;
				--num_rects_left;
			}
		}

		//prune any free rects that are completely enclosed within another
		for (int i = 0; i < free_rects_.size(); ++i) {
			for (int j = i + 1; j < free_rects_.size(); ++j) {
				//if j is enclosed in k, remove j
				if (EnclosedInRect(free_rects_[i], free_rects_[j])) {
					free_rects_.erase(free_rects_.begin() + i);
					--i;
					break;
				}
				//vice versa
				else if (EnclosedInRect(free_rects_[j], free_rects_[i])) {
					free_rects_.erase(free_rects_.begin() + j);
					--j;
				}

			}
		}
	}

	return true;
}

bool MaxRects::IntersectsRect(const Rect& new_rect, const Rect& free_rect)
{
	//separating axis theorem
	if (new_rect.x >= free_rect.x + free_rect.w || new_rect.x + new_rect.w <= free_rect.x ||
		new_rect.y >= free_rect.y + free_rect.h || new_rect.y + new_rect.h <= free_rect.y) {
		return false;
	}

	return true;
}

void MaxRects::PushSplitRects(const Rect& new_rect, const Rect& free_rect)
{
	//top rect
	if (new_rect.y > free_rect.y) {
		Rect temp = free_rect;
		temp.h = new_rect.y - free_rect.y - pixel_padding_;
		free_rects_.push_back(temp);
	}

	//bottom rect
	if (free_rect.y + free_rect.h > new_rect.y + new_rect.h) {
		Rect temp = free_rect;
		temp.y = new_rect.y + new_rect.h + pixel_padding_;
		temp.h = free_rect.y + free_rect.h - (new_rect.y + new_rect.h) - pixel_padding_;
		free_rects_.push_back(temp);
	}

	//left rect
	if (new_rect.x > free_rect.x) {
		Rect temp = free_rect;
		temp.w = new_rect.x - free_rect.x - pixel_padding_;
		free_rects_.push_back(temp);
	}

	//right rect
	if (free_rect.x + free_rect.w > new_rect.x + new_rect.w) {
		Rect temp = free_rect;
		temp.x = new_rect.x + new_rect.w + pixel_padding_;
		temp.w = free_rect.x + free_rect.w - (new_rect.x + new_rect.w) - pixel_padding_;
		free_rects_.push_back(temp);
	}
}

bool MaxRects::EnclosedInRect(const Rect& a, const Rect& b)
{
	return (a.y >= b.y && a.x >= b.x &&
		a.x + a.w <= b.x + b.w && a.y + a.h <= b.y + b.h);
}
