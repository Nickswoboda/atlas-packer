#pragma once

#include "Window.h"
#include "FileDialog.h"
#include "AtlasPacker.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

class Application
{
public:

	enum class State {
		Input,
		Output,
		Settings
	};
	enum class OutputFormat {
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
	void ImGuiErrorText(const std::string& error);

	void PushState(State state);
	void PopState();
	void Save(const std::string& save_folder);

	void UnpackInputFolders();
	unsigned int Application::CreateAtlasTexture(int image_index);
	void CreateAtlasFromCmdLine(int argc, char** argv);
	bool IsNumber(const std::string& value);

private:
	bool running_ = true;

	AtlasPacker atlas_packer_;
	ImageData image_data_;
	int atlas_index_;

	Window window_;
	FileDialog input_file_dialog_;
	FileDialog save_file_dialog_;
	int jpg_quality_ = 90;

	std::string output_directory_;
	bool changing_save_folder_ = false;
	bool max_images_exceeded_ = false;

	OutputFormat output_format_ = OutputFormat::PNG;

	std::unordered_set<std::string> input_items_;
	std::vector<std::string> unpacked_items_;

	std::stack<State> state_stack_;

	Vec2 preview_size_;
	unsigned int atlas_texture_ID_ = -1;
};