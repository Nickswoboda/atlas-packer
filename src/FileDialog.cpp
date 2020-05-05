#include "FileDialog.h"

#include <imgui.h>

#include <filesystem>
#include <iostream>

#include <Windows.h>

FileDialog::FileDialog(int width)
	: width_(width)
{
	//get list of physical drives
	constexpr DWORD bufferlength = 100;
	char drive_buffer[bufferlength];
	DWORD string_length = GetLogicalDriveStrings(bufferlength, drive_buffer);
	if (string_length > 0 && string_length <= 100) {
		char* drive = drive_buffer;
		while (*drive) {
			drives_.push_back(drive);
			drive += strlen(drive_buffer) + 1;
		}
	}
}

void FileDialog::Render()
{
	ImGui::TextWrapped(current_file_path_.c_str());

	std::filesystem::path path = current_file_path_;

	if (prev_paths_.empty()) {
		ImGui::BeginChildFrame(1, { width_ / 2.0f, 200 });
		int index = 0;
		static int selected_index = -1;
		for (const auto& drive : drives_) {
			if (ImGui::Selectable(drive.c_str(), selected_index == index, ImGuiSelectableFlags_AllowDoubleClick)) {

				if (ImGui::IsMouseDoubleClicked(0)) {
					prev_paths_.push(drive);
					current_file_path_ = drive;
				}

				selected_index = index;
				selected_path_ = drive;
			}
			++index;
		}
		ImGui::EndChildFrame();
	}
	else {
		//try/catch needed for when choosing empty media device such as dvd player
		try {
			if (!std::filesystem::exists(path)) {
				std::cout << "file path does not exist";
				return;
			}

			ImGui::BeginChildFrame(1, { width_ /2.0f, 200 });

			int index = 0;
			static int selected_index = -1;
			for (auto& p : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied)) {
				if (p.is_directory() || p.path().extension() == ".png" || p.path().extension() == ".jpg")
					if (ImGui::Selectable(p.path().filename().u8string().c_str(), selected_index == index, ImGuiSelectableFlags_AllowDoubleClick)) {

						if (ImGui::IsMouseDoubleClicked(0) && p.is_directory()) {
							prev_paths_.push(current_file_path_);
							current_file_path_ = p.path().u8string();
						}

						selected_index = index;
						selected_path_ = p.path();
					}
				
				++index;
			}

			ImGui::EndChildFrame();
		}
		catch (std::filesystem::filesystem_error & error) {};
	}

	ImGui::SameLine();
	ImGui::BeginChildFrame(2, { width_ / 2.0f, 200 });
	int index = 0;
	static int selected_index = -1;
	for (auto& item : input_items_) {

		if (ImGui::Selectable(item.c_str(), selected_index == index)) {
			selected_index = index;
			selected_input_item_ = item.c_str();
		}
		++index;
	}
	ImGui::EndChildFrame();

	if (ImGui::Button("Prev")) {
		if (!prev_paths_.empty()) {
			current_file_path_ = prev_paths_.top();
			prev_paths_.pop();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Add")) {
		if (!selected_path_.empty()) {
			input_items_.insert(selected_path_.u8string());
		}
	}

	ImGui::SameLine(width_ / 2.0f);
	if (ImGui::Button("Remove")) {
		input_items_.erase(selected_input_item_);
		selected_index = -1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear All")) {
		if (!selected_path_.empty()) {
			input_items_.clear();
		}
	}

}

