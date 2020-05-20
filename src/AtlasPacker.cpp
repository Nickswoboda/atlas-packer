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

	if (fixed_size_) {
		size_ = { max_width_, max_height_ };
	}
	else if (size_solver_ == AtlasSizeSolver::Fast) {
		size_ = EstimateAtlasSize(image_data);
	}

	if (size_solver_ == AtlasSizeSolver::BestFit) {
		GetPossibleContainers(image_data, possible_sizes_);
		for (int i = 0; i < possible_sizes_.size(); ++i) {
			if (PackAtlas(image_data, possible_sizes_[i])) {
				size_ = possible_sizes_[i];
				break;
			}

			if ( i == possible_sizes_.size() - 1) {
				return -1;
			}
		}
	}
	else {
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
	std::sort(sorted_indices.begin(), sorted_indices.end(), [&images](int i, int j) { return images.rects_[i].w * images.rects_[i].h > images.rects_[j].w * images.rects_[j].h; });

	//start with bounding box being available
	std::vector<Rect> available_rects_;
	available_rects_.push_back({ 0,0, size.x, size.y });

	while (!sorted_indices.empty()) {
		int curr_idx = sorted_indices[0];
		sorted_indices.erase(sorted_indices.begin());

		if (!available_rects_.empty()) {
			for (int i = 0; i < available_rects_.size(); ++i) {
				//if image can fit into available rect
				if (images.rects_[curr_idx].w <= available_rects_[i].w && images.rects_[curr_idx].h <= available_rects_[i].h) {

					auto& rect = images.rects_[curr_idx];
					auto& box = available_rects_[i];

					rect.x = box.x;
					rect.y = box.y;

					int width = box.w - rect.w;
					int height = box.h - rect.h;

					Rect new_rect_1;
					Rect new_rect_2;

					if (width < height) {
						new_rect_1 = { box.x + rect.w, box.y, box.w - rect.w, rect.h };
						new_rect_2 = { box.x, box.y + rect.h, box.w, box.h - rect.h };
					}
					else {
						new_rect_1 = { box.x + rect.w, box.y, box.w - rect.w, box.h };
						new_rect_2 = { box.x, box.y + rect.h, rect.w, box.h - rect.h };
					}

					available_rects_.push_back(new_rect_1);
					available_rects_.push_back(new_rect_2);

					available_rects_.erase(available_rects_.begin() + i);

					std::sort(available_rects_.begin(), available_rects_.end(), [&images](Rect a, Rect b) { return a.w * a.h < b.w* b.h; });
					break;
				}
				else if (i == (available_rects_.size() - 1)) {
					return false;
				}
			}
		}
		else {
			return false;
		}


	}

	//draw unused rects for debugging
	for (const auto& rect : available_rects_) {

		unsigned char* data = new unsigned char[(size_t)rect.w * rect.h * 4]();
		for (int row = 0; row < rect.h; ++row) {
			for (int col = 0; col < rect.w * 4; col += 4) {
				if (row < 1 || col < 1) {
					data[row * (rect.w * 4) + col] = 225;
					data[row * (rect.w * 4) + col + 1] = 225;
					data[row * (rect.w * 4) + col + 2] = 225;
					data[row * (rect.w * 4) + col + 3] = 225;
				}
				else {
					data[row * (rect.w * 4) + col] = 225;
					data[row * (rect.w * 4) + col + 1] = 0;
					data[row * (rect.w * 4) + col + 2] = 0;
					data[row * (rect.w * 4) + col + 3] = 225;
				}
			}
		}
		images.rects_[images.num_images_] = rect;
		images.data_[images.num_images_] = data;
		++images.num_images_;
	}

	return true;
}

void AtlasPacker::GetPossibleContainers(const ImageData& images, std::vector<Vec2>& possible_sizes)
{
	int min_width = 0;
	int min_height = 0;
	int max_width = 0;
	int max_height = 0;
	stats_.total_images_area = 0;
	for (int i = 0; i < images.num_images_; ++i) {
		max_width += images.rects_[i].w;
		if (images.rects_[i].w > min_width) {
			min_width = images.rects_[i].w;
		}

		stats_.total_images_area += images.rects_[i].w * images.rects_[i].h;

		max_height += images.rects_[i].h;
		if (images.rects_[i].h > min_height) {
			min_height = images.rects_[i].h;
		}
	}

	max_width = std::min(max_width, 4096);
	max_height = std::min(max_height, 4096);

	for (int w = min_width; w < max_width; ++w) {
		for (int h = min_height; h < max_height; ++h) {
			if ((w * h) > stats_.total_images_area) {
				possible_sizes.push_back({ w, h });
			}
		}
	}

	//sort by smallest area
	std::sort(possible_sizes.begin(), possible_sizes.end(), [](Vec2 a, Vec2 b) { return a.x * a.y < b.x * b.y; });
}

Vec2 AtlasPacker::EstimateAtlasSize(const ImageData& images)
{
	stats_.total_images_area = 0;

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