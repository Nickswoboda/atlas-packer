#pragma once

#include "Window.h"
#include "FileDialog.h"

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
	void Save();

	void SetKeyCallbacks();

private:
	bool running_ = true;

	int font_size_= 16;
	bool font_size_changed_ = false;

	Window window_;
	std::unique_ptr<FileDialog> file_dialog_;

	std::stack<State> state_stack_;

	int pixel_padding_ = 0;
	bool pow_of_2 = false;
	
};