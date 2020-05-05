#include "Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

Application::Application(int width, int height)
	:window_(width, height)
{
	if (!gladLoadGLLoader(GLADloadproc(glfwGetProcAddress))) {
		std::cout << "could not load GLAD";
	}

	Load();
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	if (std::filesystem::exists("assets/fonts/Roboto-Medium.ttf")) {
		io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Medium.ttf", font_size_);
	}

	ImGui_ImplGlfw_InitForOpenGL(window_.glfw_window_, true);
	ImGui_ImplOpenGL3_Init("#version 410");

	SetImGuiStyle();
	
	glfwSetWindowUserPointer(window_.glfw_window_, this);
	SetKeyCallbacks();

	file_dialog_ = std::make_unique<FileDialog>(window_.width_);

	state_stack_.push(State::Input);
}

Application::~Application()
{
	Save();
	glfwTerminate();
}

void Application::Run()
{
	while (!glfwWindowShouldClose(window_.glfw_window_) && running_)
	{
		if (Window::IsFocused()) {
			Render();
		}

		glfwPollEvents();

		//Must change font outside of ImGui Rendering
		if (font_size_changed_) {
			font_size_changed_ = false;

			ImGuiIO& io = ImGui::GetIO();
			delete io.Fonts;
			io.Fonts = new ImFontAtlas();
			io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Medium.ttf", font_size_);
			ImGui_ImplOpenGL3_CreateFontsTexture();
		}
	}

}

void Application::Update()
{
}

void Application::Render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize({ (float)window_.width_, (float)window_.height_ });
	ImGui::Begin("Atlas Packer", &running_, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);


	switch (state_stack_.top()) {
		case State::Input: RenderInputState(); break;
		case State::Output: RenderOutputState(); break;
		case State::Settings: RenderSettingsMenu(); break;
	}

	if (state_stack_.top() != State::Settings) {
		if (ImGui::Button("Settings")) {
			PushState(State::Settings);
		}
	}
	ImVec2 pos = ImGui::GetWindowPos();
	if (pos.x != 0.0f || pos.y != 0.0f) {
		ImGui::SetWindowPos({ 0.0f, 0.0f });
		window_.Move(pos.x, pos.y);
	}

	if (window_.height_ < ImGui::GetCursorPosY()) {
		window_.ResizeHeight(ImGui::GetCursorPosY());
	}
	
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(window_.glfw_window_);
}

void Application::RenderInputState()
{
	file_dialog_->Render();

	ImGui::Text("Pixel Padding: ");
	ImGui::SameLine();
	if (ImGui::InputInt("##Padding", &pixel_padding_)) {
		if (pixel_padding_ < 0) {
			pixel_padding_ = 0;
		}
		if (pixel_padding_ > 32) {
			pixel_padding_ = 32;
		}
	}

	ImGui::Text("Power of 2: ");
	ImGui::SameLine();
	ImGui::Checkbox("##Powof2", &pow_of_2);

	if (ImGui::Button("Submit")) {
		PushState(State::Output);
	}
}

void Application::RenderOutputState()
{
	ImGui::Text("Preview");


	if (ImGui::Button("Save")) {

	}

	if (ImGui::Button("Try Again")) {
		PopState();
	}

}

void Application::RenderSettingsMenu()
{
	ImGui::TextWrapped("Font Size");
	ImGui::PushItemWidth(window_.width_);
	if (ImGui::DragInt("##font", &font_size_, 1.0f, 10, 32)) {
		font_size_changed_ = true;
	}
	ImGui::TextWrapped("Window Width");
	if (ImGui::DragInt("##width", &window_.width_, 1.0f, 100, 1000)) {
		window_.UpdateSize();

		if (file_dialog_ != nullptr) {
			file_dialog_->width_ = window_.width_;
		}
	}
	ImGui::PopItemWidth();

	if (ImGui::Button("Save")) {
		Save();
	}


	if (ImGui::Button("Return")) {
		PopState();
	}

}

void Application::PushState(State state)
{
	state_stack_.push(state);
}

void Application::PopState()
{
	state_stack_.pop();
}

void Application::SetImGuiStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(2, 2);

	style->WindowRounding = 0.0f;
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.2f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
}

void Application::Load()
{
}

void Application::Save()
{
}

void Application::SetKeyCallbacks()
{
	glfwSetKeyCallback(window_.glfw_window_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {

		auto* app = (Application*)glfwGetWindowUserPointer(Window::glfw_window_);
	});
}


