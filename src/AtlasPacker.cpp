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
	else {
		GetPossibleContainers(image_data, possible_containers_);
		if (!possible_containers_.empty()) {
			size_ = possible_containers_[0];
		}
	}

	if (size_solver_ == AtlasSizeSolver::BestFit) {
		for (const auto& size : possible_containers_) {
			if (PackAtlas(image_data, size)) {
				break;
			}
		}
		return -1;
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

	possible_containers_.clear();

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
	return false;
}

void AtlasPacker::GetPossibleContainers(const ImageData& images, std::vector<Vec2>& possible_containers)
{
	possible_containers.push_back({ 500, 500 });
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