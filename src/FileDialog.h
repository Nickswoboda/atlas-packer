#pragma once

#include <string>
#include <vector>
#include <stack>
#include <filesystem>

class FileDialog
{
public:

	FileDialog(int width);

	void GoBack();
	void Render();

	int width_;

	std::string current_file_path_ = "/";
	std::stack<std::string> prev_paths_;
	std::vector<std::string> drives_;
	std::filesystem::path selected_path_;
};