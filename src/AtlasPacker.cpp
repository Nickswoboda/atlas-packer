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

	if (size_solver_ == SizeSolver::BestFit) {

		//get heap of Vec2 sizes with x = min image width and y = 0 to 4096
		GetPossibleContainers(image_data, possible_sizes_);

		//sort heap by min area, try top of heap. If it does not work, increase the width and push back into heap. repeat until successful 
		while (!PackAtlas(image_data, size_)) {

			if (force_square_ || pow_of_2_) {
				if (possible_sizes_.empty()) {
					return -1;
				}
				std::pop_heap(possible_sizes_.begin(), possible_sizes_.end(), [](Vec2 a, Vec2 b) { return a.x * a.y > b.x * b.y; });
				size_ = possible_sizes_.back();
				possible_sizes_.pop_back();
			}
			else {
				++size_.x;
				//do not put back into heap if it will be larger than the maximum width of 4096
				if (!(size_.x > width_)) {
					possible_sizes_.push_back(size_);
					std::push_heap(possible_sizes_.begin(), possible_sizes_.end(), [](Vec2 a, Vec2 b) { return a.x * a.y > b.x * b.y; });
				}

				if (possible_sizes_.empty()) {
					return -1;
				}

				std::pop_heap(possible_sizes_.begin(), possible_sizes_.end(), [](Vec2 a, Vec2 b) { return a.x * a.y > b.x * b.y; });
				size_ = possible_sizes_.back();
				possible_sizes_.pop_back();
			}

		}
	}
	else { 
		if (size_solver_ == SizeSolver::Fixed) {
			size_ = { width_, height_ };
		}
		//if not fixed, then fast
		else {
			size_ = EstimateAtlasSize(image_data);
		}

		while (!PackAtlas(image_data, size_)) {

			if (size_solver_ == SizeSolver::Fixed) {
				return -1;
			}
			else{
				//every iteration increase either by 64px or double size if pow_of_two is enabled
				size_.x == size_.y ? (size_.y += pow_of_2_ ? size_.y : 32) : size_.x = size_.y;
				if (force_square_) {
					size_.x = size_.y;
				}

				if (size_.x > width_ || size_.y > height_) {
					return -1;
				}
			}
		}
	}

	std::chrono::steady_clock::time_point end_time = std::chrono::high_resolution_clock::now();

	stats_.atlas_area = size_.x * size_.y;
	stats_.unused_area = stats_.atlas_area - stats_.total_images_area;
	stats_.packing_efficiency = (stats_.total_images_area / (float)stats_.atlas_area) * 100;
	stats_.time_elapsed_in_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

	//contains x, y, w, h of all individual textures in atlas
	metadata_ = GetAtlasMetadata(image_data);

	WriteAtlasImageData(image_data, size_.x, size_.y);

	possible_sizes_.clear();

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
	//sort indices of sizes as if they were sorted by height, but without actually sorting the underlying structure
	std::vector<int> sort_indices(images.num_images_);
	std::iota(sort_indices.begin(), sort_indices.end(), 0);
	std::sort(sort_indices.begin(), sort_indices.end(), [&images](int i, int j) { return images.rects_[i].h > images.rects_[j].h; });

	int pen_x = 0, pen_y = 0;
	int next_pen_y = images.rects_[sort_indices[0]].h;

	for (int i = 0; i < images.num_images_; ++i) {

		while (pen_x + images.rects_[sort_indices[i]].w >= size.x) {
			pen_x = 0;
			pen_y += next_pen_y + pixel_padding_;
			next_pen_y = images.rects_[sort_indices[i]].h;

			//unable to fit everything in atlas
			if (pen_y + images.rects_[sort_indices[i]].h >= size.y) {
				return false;
			}
		}

		images.rects_[sort_indices[i]].x = pen_x;
		images.rects_[sort_indices[i]].y = pen_y;

		pen_x += images.rects_[sort_indices[i]].w + pixel_padding_;
	}

	return true;
}

bool AtlasPacker::PackAtlasMaxRects(ImageData& images, Vec2 size)
{
	//sort indices of sizes as if they were sorted by height, but without actually sorting the underlying structure
	std::vector<int> sorted_indices(images.num_images_);
	std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
	std::sort(sorted_indices.begin(), sorted_indices.end(), [&images](int i, int j) { return images.rects_[i].h > images.rects_[j].h; });

	//start with whole atlas being available
	std::vector<Rect> free_rects;
	free_rects.reserve(images.num_images_ * 2);
	free_rects.push_back({ 0,0, size.x, size.y });

	int max_num_free_rects = 1;

	while (!sorted_indices.empty()) {

		if (free_rects.empty()) {
			return false; 
		}

		//pop first image
		int curr_idx = sorted_indices[0];
		sorted_indices.erase(sorted_indices.begin());

		int best_short_side_fit = MAX_DIMENSIONS;
		Vec2 bssf_pos;
		for (int i = 0; i < free_rects.size(); ++i) {

			//find best short side fit
			//if image can fit into available rect
			if (images.rects_[curr_idx].w <= free_rects[i].w && images.rects_[curr_idx].h <= free_rects[i].h){

				int leftover_width = free_rects[i].w - images.rects_[curr_idx].w;
				int leftover_height = free_rects[i].h - images.rects_[curr_idx].h;
				int shortest_side = std::min(leftover_width, leftover_height);

				if (shortest_side < best_short_side_fit) {
					best_short_side_fit = shortest_side;
					bssf_pos.x = free_rects[i].x;
					bssf_pos.y = free_rects[i].y;
				}
			}
		}

		//didnt find any fits
		if (best_short_side_fit == MAX_DIMENSIONS) {
			return false;
		}

		images.rects_[curr_idx].x = bssf_pos.x;
		images.rects_[curr_idx].y = bssf_pos.y;


		//split intersected free rects into at most 4 new smaller rects

		//used to not waste time going over the new split rects that are added
		int num_rects_left = free_rects.size();
		for (int i = 0; i < num_rects_left; ++i) {
			if (IntersectsRect(images.rects_[curr_idx], free_rects[i])){
				PushSplitRects(free_rects, images.rects_[curr_idx], free_rects[i]);

				free_rects.erase(free_rects.begin() + i);
				--i;
				--num_rects_left;
			}
		}

		//prune any free rects that are completely enclosed within another
		for (int j = 0; j < free_rects.size(); ++j) {
			for (int k = j + 1; k < free_rects.size(); ++k) {
				//if j is enclosed in k, remove j
				if (EnclosedInRect(free_rects[j], free_rects[k])){
					free_rects.erase(free_rects.begin() + j);
					--j;
					break;
				}
				//vice versa
				else if (EnclosedInRect(free_rects[k], free_rects[j])) {
					free_rects.erase(free_rects.begin() + k);
					--k;
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
		stats_.total_images_area += images.rects_[i].w * images.rects_[i].h;
	}

	max_height = std::min(max_height, height_);

	if (pow_of_2_) {
		for (int w = 1; w < width_; w *= 2) {
			if (force_square_ && w * w > stats_.total_images_area) {
				possible_sizes.push_back({ w, w });
			}
			else {
				for (int h = 1; h < height_; h *= 2) {
					if (w * h > stats_.total_images_area) {
						possible_sizes.push_back({ w, h });
					}
				}
			}
		}
	}
	else {
		for (int h = min_height; h < max_height; ++h) {

			if (force_square_ && h * h > stats_.total_images_area) {
				possible_sizes.push_back({ h, h });
			}
			else {
				int w = min_width;
				while (w * h < stats_.total_images_area) {
					++w;
				}
				if (w <= width_) {
					if (force_square_ && w != h) {
						continue;
					}
					possible_sizes.push_back({ w, h });
				}
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

Vec2 AtlasPacker::EstimateAtlasSize(const ImageData& images)
{
	for (int i = 0; i < images.num_images_; ++i){
		stats_.total_images_area += images.rects_[i].w * images.rects_[i].h;
	}

	Vec2 size{ 64, 64 };
	while (size.x * size.y < stats_.total_images_area) {
		if (pow_of_2_) {
			size.x == size.y ? size.y *= 2 : size.x = size.y;
		}
		else {
			size.x == size.y ? size.y += 32 : size.x = size.y;
		}
	}
	return size;
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
