#include "AtlasPacker.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <numeric>

void AtlasPacker::WriteAtlasImageData(ImageData& images, int width, int height)
{
	int channels = 4;
	int atlas_pitch = width * channels;

	unsigned char* pixels = new unsigned char[(size_t)height * atlas_pitch]();

	for (int i = 0; i < images.num_images_; ++i) {

		int image_pitch = images.rects_[i].w * channels;

		int pen_x = images.rects_[i].x * channels;
		int pen_y = images.rects_[i].y;

		for (int row = 0; row < images.rects_[i].h; ++row) {
			for (int col = 0; col < image_pitch; ++col) {
				int x = pen_x + col;
				int y = pen_y + row;
				pixels[y * (atlas_pitch)+x] = images.data_[i][row * (image_pitch)+col];
			}
		}
	}

	//set atlas data to be at the end of all images
	images.rects_[images.num_images_] = { 0, 0, width, height };
	images.data_[images.num_images_] = pixels;
}

int AtlasPacker::CreateAtlas(ImageData& image_data)
{
	std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();

	stats_.total_images_area = 0;

	//Get heap of all possible sizes sorted by ascending area. If size solver is best fit and neither force square or power of 2, instead of storing all possible combinations, 
	//only store all possible heights with a minimum width. After each iteration, increase the width by 1 and push back into heap. Greatly reducing space complexity.
	GetPossibleContainers(image_data, possible_sizes_);
	sorted_indices_ = GetSortedIndices(image_data);

	while (!PackAtlas(image_data, size_)) {

		//increase width and push back into heap
		if (size_solver_ == SizeSolver::BestFit && !force_square_ && !pow_of_2_) {
			++size_.x;
			//do not put back into heap if it will be larger than the maximum width of 4096
			if (!(size_.x > max_width_)) {
				possible_sizes_.push_back(size_);
				std::push_heap(possible_sizes_.begin(), possible_sizes_.end(), [](Vec2 a, Vec2 b) { return a.x * a.y > b.x * b.y; });
			}
		}

		if (possible_sizes_.empty()) {
			return -1;
		}
		//pop next smallest area
		std::pop_heap(possible_sizes_.begin(), possible_sizes_.end(), [](Vec2 a, Vec2 b) { return a.x * a.y > b.x * b.y; });
		size_ = possible_sizes_.back();
		possible_sizes_.pop_back();
	}

	std::chrono::steady_clock::time_point end_time = std::chrono::high_resolution_clock::now();
	stats_.time_elapsed_in_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
	stats_.atlas_area = size_.x * size_.y;
	stats_.unused_area = stats_.atlas_area - stats_.total_images_area;
	stats_.packing_efficiency = (stats_.total_images_area / (float)stats_.atlas_area) * 100;

	//contains x, y, w, h of all individual textures in atlas
	metadata_ = GetAtlasMetadata(image_data);

	WriteAtlasImageData(image_data, size_.x, size_.y);

	possible_sizes_.clear();
	size_ = { 0,0 };

	return image_data.num_images_;
}

std::string AtlasPacker::GetAtlasMetadata(const ImageData& images)
{
	std::stringstream data;
	for (int i = 0; i < images.num_images_; ++i) {
		data << images.paths_[i] << ", ";
		data << "x pos: " << images.rects_[i].x << ", ";
		data << "y pos: " << images.rects_[i].y << ", ";
		data << "width: " << images.rects_[i].w << ", ";
		data << "height: " << images.rects_[i].h << "\n";
	}

	return data.str();
}

bool AtlasPacker::PackAtlas(ImageData& images, Vec2 size)
{
	return algo_ == Algorithm::Shelf ? PackAtlasShelf(images, size) : PackAtlasMaxRects(images, size);
}

bool AtlasPacker::PackAtlasShelf(ImageData& images, Vec2 size)
{

	int pen_x = 0, pen_y = 0;
	int next_pen_y = images.rects_[sorted_indices_[0]].h;

	for (int i = 0; i < images.num_images_; ++i) {

		while (pen_x + images.rects_[sorted_indices_[i]].w >= size.x) {
			pen_x = 0;
			pen_y += next_pen_y + pixel_padding_;
			next_pen_y = images.rects_[sorted_indices_[i]].h;

			//unable to fit everything in atlas
			if (pen_y + images.rects_[sorted_indices_[i]].h >= size.y) {
				return false;
			}
		}

		images.rects_[sorted_indices_[i]].x = pen_x;
		images.rects_[sorted_indices_[i]].y = pen_y;

		pen_x += images.rects_[sorted_indices_[i]].w + pixel_padding_;
	}

	return true;
}

bool AtlasPacker::PackAtlasMaxRects(ImageData& images, Vec2 size)
{
	std::vector<Rect> free_rects;
	free_rects.reserve(images.num_images_ * 2);
	//start with whole atlas being available
	free_rects.push_back({ 0,0, size.x, size.y });

	for (int image = 0; image < images.num_images_; ++image){

		if (free_rects.empty()) {
			return false; 
		}

		int curr_idx = sorted_indices_[image];

		int best_short_side_fit = MAX_DIMENSIONS;
		int best_fit_index = 0;
		for (int i = 0; i < free_rects.size(); ++i) {
				int leftover_width = free_rects[i].w - images.rects_[curr_idx].w;
				int leftover_height = free_rects[i].h - images.rects_[curr_idx].h;
				int shortest_side = std::min(leftover_width, leftover_height);

				//if shortest side < 0 then image did not fit into free rect
				if (shortest_side >= 0 && shortest_side < best_short_side_fit) {
					best_short_side_fit = shortest_side;
					best_fit_index = i;
				}
		}

		//didnt find any fits
		if (best_short_side_fit == MAX_DIMENSIONS) {
			return false;
		}

		images.rects_[curr_idx].x = free_rects[best_fit_index].x;
		images.rects_[curr_idx].y = free_rects[best_fit_index].y;

		//used to not waste time going over the new split rects that are added
		int num_rects_left = free_rects.size();
		for (int i = 0; i < num_rects_left; ++i) {
			if (IntersectsRect(images.rects_[curr_idx], free_rects[i])){
				//split intersected free rects into at most 4 new smaller rects
				PushSplitRects(free_rects, images.rects_[curr_idx], free_rects[i]);

				free_rects.erase(free_rects.begin() + i);
				--i;
				--num_rects_left;
			}
		}

		//prune any free rects that are completely enclosed within another
		for (int i = 0; i < free_rects.size(); ++i) {
			for (int j = i + 1; j < free_rects.size(); ++j) {
				//if j is enclosed in k, remove j
				if (EnclosedInRect(free_rects[i], free_rects[j])){
					free_rects.erase(free_rects.begin() + i);
					--i;
					break;
				}
				//vice versa
				else if (EnclosedInRect(free_rects[j], free_rects[i])) {
					free_rects.erase(free_rects.begin() + j);
					--j;
				}
		
			}
		}
	}

	return true;
}

bool AtlasPacker::IntersectsRect(const Rect& new_rect, const Rect& free_rect)
{
	//separating axis theorem
	if (new_rect.x >= free_rect.x + free_rect.w || new_rect.x + new_rect.w <= free_rect.x ||
		new_rect.y >= free_rect.y + free_rect.h || new_rect.y + new_rect.h <= free_rect.y) {
		return false;
	}

	return true;
}

void AtlasPacker::GetPossibleContainers(const ImageData& images, std::vector<Vec2>& possible_sizes)
{
	for (int i = 0; i < images.num_images_; ++i) {
		stats_.total_images_area += images.rects_[i].w * images.rects_[i].h;
	}

	switch (size_solver_) {
		case SizeSolver::Fixed: { possible_sizes_.push_back({ max_width_, max_height_ }); break; }
		
		case SizeSolver::Fast: {

			Vec2 size{ 32, 32 };
			while (size.x <= max_width_ && size.y <= max_height_) {
				
				size.x == size.y ? (size.x += pow_of_2_ ? size.x : 32) : size.y = size.x;
				if (force_square_) { 
					size.y = size.x; 
				}

				if (size.x * size.y > stats_.total_images_area) {
					possible_sizes.push_back(size);
				}
			}
			break;
		}

		case SizeSolver::BestFit: {

			int min_width = 0;
			int min_height = 0;
			int max_height = 0;
			for (int i = 0; i < images.num_images_; ++i) {
				if (images.rects_[i].w > min_width) {
					min_width = images.rects_[i].w;
				}

				max_height += images.rects_[i].h;
				if (images.rects_[i].h > min_height) {
					min_height = images.rects_[i].h;
				}
			}
			max_height = std::min(max_height, max_height_);

			for (int h = 1; h < max_height;) {
				if (force_square_){
					if (h * h > stats_.total_images_area && h >= min_height) {
						possible_sizes.push_back({ h, h });
					}
				}
				else{
					int w = 1;
					while (w * h < stats_.total_images_area || w < min_width) {
						pow_of_2_ ? w *= 2 : ++w;
					}
					if (w <= max_width_ && h >= min_height) {
						possible_sizes.push_back({ w, h });
					}
				}
				pow_of_2_ ? h *= 2 : ++h;
			}
		}
	}

	//sort by smallest area and pop smallest fit into size_
	if (possible_sizes.empty()) {
		return;
	}

	std::make_heap(possible_sizes.begin(), possible_sizes.end(), [](Vec2 a, Vec2 b) { return a.x * a.y > b.x * b.y; });
	std::pop_heap(possible_sizes.begin(), possible_sizes.end(), [](Vec2 a, Vec2 b) { return a.x * a.y > b.x * b.y; });
	size_ = possible_sizes.back();
	possible_sizes.pop_back();
}

void AtlasPacker::PushSplitRects(std::vector<Rect>& rects, const Rect& new_rect, const Rect free_rect)
{
	//top rect
	if (new_rect.y > free_rect.y){
		Rect temp = free_rect;
		temp.h = new_rect.y - free_rect.y - pixel_padding_;
		rects.push_back(temp);
	}

	//bottom rect
	if (free_rect.y + free_rect.h > new_rect.y + new_rect.h) {
		Rect temp = free_rect;
		temp.y = new_rect.y + new_rect.h + pixel_padding_;
		temp.h = free_rect.y + free_rect.h - (new_rect.y + new_rect.h) - pixel_padding_;
		rects.push_back(temp);
	}

	//left rect
	if (new_rect.x > free_rect.x) {
		Rect temp = free_rect;
		temp.w = new_rect.x - free_rect.x - pixel_padding_;
		rects.push_back(temp);
	}

	//right rect
	if (free_rect.x + free_rect.w > new_rect.x + new_rect.w) {
		Rect temp = free_rect;
		temp.x = new_rect.x + new_rect.w + pixel_padding_;
		temp.w = free_rect.x + free_rect.w - (new_rect.x + new_rect.w) - pixel_padding_;
		rects.push_back(temp);
	}
	
}

bool AtlasPacker::EnclosedInRect(const Rect& a, const Rect& b)
{
	return (a.y >= b.y && a.x >= b.x &&
			a.x + a.w <= b.x + b.w && a.y + a.h <= b.y + b.h);
}

//sort image data without affected underlying structure
std::vector<int> AtlasPacker::GetSortedIndices(const ImageData& images)
{
	std::vector<int> sorted_indices(images.num_images_);
	std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
	std::sort(sorted_indices.begin(), sorted_indices.end(), [&images](int i, int j) { return images.rects_[i].h > images.rects_[j].h; });

	return sorted_indices;
}
