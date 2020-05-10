#include "FileDialog.h"

#include <imgui.h>

#include <filesystem>
#include <iostream>

#include <Windows.h>

FileDialog::FileDialog(int width, int height)
	: width_(width), height_(height)
{
	//get list of physical drives
	constexpr DWORD buffer_length = 100;
	char drive_buffer[buffer_length];
	DWORD string_length = GetLogicalDriveStrings(buffer_length, drive_buffer);
	if (string_length > 0 && string_length <= buffer_length) {
		char* drive = drive_buffer;
		while (*drive) {
			drives_.push_back(drive);
			drive += strlen(drive_buffer) + 1;
		}
	}
}

void FileDialog::GoBack()
{
	if (!prev_paths_.empty()) {
		current_dir_path_ = prev_paths_.top();
		prev_paths_.pop();
		if (prev_paths_.size() == 1) {
			current_dir_path_ += "/";
		}
		current_dir_files_ = GetDirectoryFiles(current_dir_path_);
	}
}

std::vector<std::string> FileDialog::GetDirectoryFiles(const std::filesystem::path& directory)
{
	std::vector<std::string> files;
	//try/catch needed for when choosing empty media device such as dvd player
	try {
		for (auto& file : std::filesystem::directory_iterator(directory, std::filesystem::directory_options::skip_permission_denied)) {
			if (file.is_directory() || file.path().extension() == ".png" || file.path().extension() == ".jpg") {
				files.push_back(file.path().filename().u8string());
			}
		}
	}
	catch (std::filesystem::filesystem_error& error) {};
	return files;
}

void FileDialog::Render()
{
	ImGui::TextWrapped(current_dir_path_.c_str());

	if (prev_paths_.empty()) {
		ImGui::BeginChildFrame(1, { (float)width_, (float)height_ });
		int index = 0;
		static int selected_index = -1;
		for (const auto& drive : drives_) {
			if (ImGui::Selectable(drive.c_str(), selected_index == index, ImGuiSelectableFlags_AllowDoubleClick)) {

				if (ImGui::IsMouseDoubleClicked(0)) {
					std::filesystem::path drive_path(drive);
					current_dir_files_ = GetDirectoryFiles(drive_path);
					prev_paths_.push("/");
					current_dir_path_ = drive_path.root_name().u8string();
				}

				selected_index = index;
				selected_path_ = drive;
			}
			++index;
		}
		ImGui::EndChildFrame();
	}
	else {
		ImGui::BeginChildFrame(1, { (float)width_, (float)height_ });

		int index = 0;
		static int selected_index = -1;
		for (const auto& file : current_dir_files_) {
			if (ImGui::Selectable(file.c_str(), selected_index == index, ImGuiSelectableFlags_AllowDoubleClick)) {

				if (ImGui::IsMouseDoubleClicked(0)) {
					std::filesystem::path path(current_dir_path_ + "/" + file);
					if (std::filesystem::is_directory(path)) {

						prev_paths_.push(current_dir_path_);
						current_dir_path_ += "/" + file;

						//invalidates all previous files in loop
						current_dir_files_ = GetDirectoryFiles(path);
						ImGui::EndChildFrame();
						return;
					}
				}

				selected_index = index;
				selected_path_ = current_dir_path_ + "/" + file;

			}
			++index;
		}

		ImGui::EndChildFrame();
	}
}



