#include "AtlasPacker.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <numeric>

void CreateAtlasImageData(ImageData& images, int width, int height)
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

	if (size_solver_ == AtlasSizeSolver::BestFit) {

		//get heap of Vec2 sizes with x = min image width and y = 0 to 4096
		GetPossibleContainers(image_data, possible_sizes_);

		//sort heap by min area, try top of heap. If it does not work, increase the width and push back into heap. repeat until successful 
		while (!PackAtlas(image_data, size_)) {

			++size_.x;
			//do not put back into heap if it will be larger than the maximum width of 4096
			if (!(size_.x > max_width_)) {
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
	else { 
		if (fixed_size_) {
			size_ = { max_width_, max_height_ };
		}
		else if (size_solver_ == AtlasSizeSolver::Fast) {
			size_ = EstimateAtlasSize(image_data);
		}

		while (!PackAtlas(image_data, size_)) {

			if (fixed_size_) {
				return -1;
			}
			else{
				//every iteration increase either by 64px or double size if pow_of_two is enabled
				size_.x == size_.y ? (size_.x += pow_of_2_ ? size_.x : 64) : size_.y = size_.x;
				if (force_square_) {
					size_.y = size_.x;
				}

				if (size_.x > max_width_ || size_.y > max_height_) {
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

	CreateAtlasImageData(image_data, size_.x, size_.y);

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
	std::vector<Rect> free_rects_;
	free_rects_.push_back({ 0,0, size.x, size.y });

	while (!sorted_indices.empty()) {

		if (free_rects_.empty()) { 
			return false; 
		}

		//pop first image
		int curr_idx = sorted_indices[0];
		sorted_indices.erase(sorted_indices.begin());

		for (int i = 0; i < free_rects_.size(); ++i) {
			//if image can fit into available rect
			if (images.rects_[curr_idx].w <= free_rects_[i].w && images.rects_[curr_idx].h <= free_rects_[i].h){

				images.rects_[curr_idx].x = free_rects_[i].x;
				images.rects_[curr_idx].y = free_rects_[i].y;
				break;
			}
			
			//if reached the end of list, can not fit in any available rects
			else if (i == (free_rects_.size() - 1)) {
				return false;
			}
		}

		//split intersected free rects into at most 4 new smaller rects

		//used to not waste time going over the new split rects that are added
		int num_rects_left = free_rects_.size();
		for (int i = 0; i < num_rects_left; ++i) {
			if (IntersectsRect(images.rects_[curr_idx], free_rects_[i])){
				auto split_rects = GetNewSplitRects(images.rects_[curr_idx], free_rects_[i]);
				free_rects_.insert(free_rects_.end(), split_rects.begin(), split_rects.end());
				free_rects_.erase(free_rects_.begin() + i);
				--i;
				--num_rects_left;
			}
		}

		//prune any free rects that are completely enclosed within another
		for (int j = 0; j < free_rects_.size(); ++j) {
			for (int k = j + 1; k < free_rects_.size(); ++k) {
				//if j is enclosed in k, remove j
				if (EnclosedInRect(free_rects_[j], free_rects_[k])){
					free_rects_.erase(free_rects_.begin() + j);
					--j;
					break;
				}
				//vice versa
				else if (EnclosedInRect(free_rects_[k], free_rects_[j])) {
					free_rects_.erase(free_rects_.begin() + k);
					--k;
				}
		
			}
		}

		std::sort(free_rects_.begin(), free_rects_.end(), [](Rect a, Rect b) { return  a.w * a.h < b.w* b.h;  });
	}

	//for (const auto& rect : free_rects_) {
	//
	//	unsigned char* data = new unsigned char[(size_t)rect.w * rect.h * 4]();
	//	for (int row = 0; row < rect.h; ++row) {
	//		for (int col = 0; col < rect.w * 4; col += 4) {
	//			if (row < 1 || col < 1) {
	//				data[row * (rect.w * 4) + col] = 225;
	//				data[row * (rect.w * 4) + col + 1] = 225;
	//				data[row * (rect.w * 4) + col + 2] = 225;
	//				data[row * (rect.w * 4) + col + 3] = 225;
	//			}
	//			else {
	//				data[row * (rect.w * 4) + col] = 225;
	//				data[row * (rect.w * 4) + col + 1] = 0;
	//				data[row * (rect.w * 4) + col + 2] = 0;
	//				data[row * (rect.w * 4) + col + 3] = 225;
	//			}
	//		}
	//	}
	//	images.rects_[images.num_images_] = rect;
	//	images.data_[images.num_images_] = data;
	//	++images.num_images_;
	//}

	return true;
}

bool AtlasPacker::IntersectsRect(Rect& new_rect, Rect& free_rect)
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

	max_height = std::min(max_height, max_height_);

	for (int h = min_height; h < max_height; ++h) {
		//min_width *3 so that you wont have 1 long texture i.e 16x4096 if all tiles are 16 px wide
		int w = min_width;
		while (w * h < stats_.total_images_area) {
			++w;
		}
		if (w <= max_width_) {
			possible_sizes.push_back({ w, h });
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

	Vec2 size{ 16, 16 };
	while (size.x * size.y < stats_.total_images_area) {
		if (pow_of_2_) {
			size.x *= 2;
			size.y *= 2;
		}
		else {
			size.x += 2;
			size.y += 2;
		}
	}
	return size;
}

std::vector<Rect> AtlasPacker::GetNewSplitRects(Rect& new_rect, Rect& free_rect)
{
	std::vector<Rect> split_rects;

	//top rect
	if (new_rect.y > free_rect.y){
		Rect temp = free_rect;
		temp.h = new_rect.y - free_rect.y;
		split_rects.push_back(temp);
	}

	//bottom rect
	if (free_rect.y + free_rect.h > new_rect.y + new_rect.h) {
		Rect temp = free_rect;
		temp.y = new_rect.y + new_rect.h;
		temp.h = free_rect.y + free_rect.h - (new_rect.y + new_rect.h);
		split_rects.push_back(temp);
	}

	//left rect
	if (new_rect.x > free_rect.x) {
		Rect temp = free_rect;
		temp.w = new_rect.x - free_rect.x;
		split_rects.push_back(temp);
	}

	//right rect
	if (free_rect.x + free_rect.w > new_rect.x + new_rect.w) {
		Rect temp = free_rect;
		temp.x = new_rect.x + new_rect.w;
		temp.w = free_rect.x + free_rect.w - (new_rect.x + new_rect.w);
		split_rects.push_back(temp);
	}

	return split_rects;
}

bool AtlasPacker::EnclosedInRect(Rect& a, Rect& b)
{
	return (a.x >= b.x && a.x + a.w <= b.x + b.w &&
			a.y >= b.y && a.y + a.h <= b.y + b.h);
}
