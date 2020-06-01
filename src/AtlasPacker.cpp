#include "AtlasPacker.h"

#include "MaxRects.h"

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
	return algo_ == Algorithm::Shelf ? PackAtlasShelf(images, size) : MaxRects::PackAtlas(images, size, sorted_indices_, pixel_padding_);
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

//sort image data without affected underlying structure
std::vector<int> AtlasPacker::GetSortedIndices(const ImageData& images)
{
	std::vector<int> sorted_indices(images.num_images_);
	std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
	std::sort(sorted_indices.begin(), sorted_indices.end(), [&images](int i, int j) { return images.rects_[i].h > images.rects_[j].h; });

	return sorted_indices;
}
