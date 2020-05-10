#pragma once

#include <string>
#include <vector>
#include <stack>
#include <filesystem>

class FileDialog
{
public:

	explicit FileDialog(int width, int height);

	void GoBack();
	std::vector<std::string> GetDirectoryFiles(const std::filesystem::path& directory);
	void Render();

	int width_;
	int height_;

	std::string current_dir_path_ = "/";
	std::stack<std::string> prev_paths_;
	std::vector<std::string> drives_;
	std::string selected_path_;
	std::vector<std::string> current_dir_files_;
};