#pragma once

#include "Window.h"
#include "FileDialog.h"
#include "ImageData.h"

#include <json.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>


struct Vec2
{
	int x = 0;
	int y = 0;
};

class Application
{
public:

	enum class State {
		Input,
		Output,
		Settings
	};
	enum class SaveFileFormat {
		PNG,
		JPG
	};

	Application(int width, int height);
	~Application();

	void SetImGuiStyle();

	void Run();
	void Render();

	void RenderInputState();
	void RenderOutputState();
	void RenderSettingsMenu();
	void ImGuiErrorText(const std::string& error);

	void PushState(State state);
	void PopState();
	void Save(const std::string& save_folder);

	void UnpackInputFolders();
	unsigned int CreateAtlas(const std::unordered_set<std::string>& paths, int padding, bool pow_of_2);
	nlohmann::json CreateJsonFile(const std::vector<ImageData>& images, std::unordered_map<std::string, Vec2> placement);
	std::unordered_map<std::string, Vec2> GetTexturePlacements(const std::vector<ImageData>& images, int width, int height, int padding);

private:
	bool running_ = true;

	int font_size_= 16;
	bool font_size_changed_ = false;

	Window window_;
	FileDialog input_file_dialog_;
	FileDialog save_file_dialog_;
	int jpg_quality_ = 90;

	std::string save_folder_path_;
	bool changing_save_folder_ = false;

	SaveFileFormat save_file_format_ = SaveFileFormat::PNG;

	std::unordered_set<std::string> input_items_;
	std::unordered_set<std::string> unpacked_items_;

	std::stack<State> state_stack_;

	int pixel_padding_ = 0;
	bool pow_of_2_ = false;

	unsigned int atlas_texture_ID_ = -1;
	ImageData atlas_;

	nlohmann::json output_json_;
};