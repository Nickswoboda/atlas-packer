#pragma once

#include "Window.h"
#include "FileDialog.h"

#include "ImageData.h"

#include <string>

class Application
{
public:

	enum class State {
		Input,
		Output,
		Settings
	};

	Application(int width, int height);
	~Application();

	void Load();
	void SetImGuiStyle();

	void Run();
	void Update();
	void Render();

	void RenderInputState();
	void RenderOutputState();
	void RenderSettingsMenu();

	void PushState(State state);
	void PopState();
	void Save(const std::string& save_path);

	void SetKeyCallbacks();

	unsigned int CreateAtlas(const std::unordered_set<std::string>& paths, int padding, bool pow_of_2);

private:
	bool running_ = true;

	int font_size_= 16;
	bool font_size_changed_ = false;

	Window window_;
	std::unique_ptr<FileDialog> file_dialog_;

	std::stack<State> state_stack_;

	int pixel_padding_ = 0;
	bool pow_of_2_ = false;

	
	unsigned int atlas_texture_ID_ = -1;
	ImageData atlas_;
	
	std::vector<unsigned int> temp_images_;
};