#pragma once

#include <string>
#include <vector>
#include <stack>
#include <filesystem>
#include <unordered_set>

class FileDialog
{
public:

	FileDialog(int width);

	void Render();

	int width_;

	std::string current_file_path_ = "/";
	std::stack<std::string> prev_paths_;
	std::vector<std::string> drives_;
	std::filesystem::path selected_path_;

	std::unordered_set<std::string> input_items_;
	std::string selected_input_item_;
};