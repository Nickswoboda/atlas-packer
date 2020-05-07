#include "Application.h"

#include "ImageData.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


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

	input_file_dialog_ = std::make_unique<FileDialog>(window_.width_);
	save_file_dialog_ = std::make_unique<FileDialog>(window_.width_);

	state_stack_.push(State::Input);
}

Application::~Application()
{
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
	input_file_dialog_->Render();

	ImGui::SameLine();
	ImGui::BeginChildFrame(2, { window_.width_ / 2.0f, 200 });
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
		input_file_dialog_->GoBack();
	}

	ImGui::SameLine();
	if (ImGui::Button("Add")) {
		if (!input_file_dialog_->selected_path_.empty()) {
			input_items_.insert(input_file_dialog_->selected_path_.u8string());
		}
	}

	ImGui::SameLine(window_.width_ / 2.0f);
	if (ImGui::Button("Remove")) {
		input_items_.erase(selected_input_item_);
		selected_index = -1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear All")) {
		if (!input_file_dialog_->selected_path_.empty()) {
			input_items_.clear();
		}
	}

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
	ImGui::Checkbox("##Powof2", &pow_of_2_);

	if (ImGui::Button("Submit")) {
		UnpackFolders();
		atlas_texture_ID_ = CreateAtlas(input_items_, pixel_padding_, pow_of_2_);
		PushState(State::Output);
	}
}

void Application::RenderOutputState()
{
	ImGui::Text("Preview");

	ImGui::Image((void*)(intptr_t)atlas_texture_ID_, { (float)atlas_.width_, (float)atlas_.height_ }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, { 1,1,1,1 });


	ImGui::Separator();
	ImGui::Text("Save Folder Path: %s", save_folder_path_.c_str());
	ImGui::SameLine();
	if (ImGui::Button("Change")){
		changing_save_folder_ = true;
	}

	if (changing_save_folder_) {
		save_file_dialog_->Render();

		if (ImGui::Button("Prev")) {
			save_file_dialog_->GoBack();
		}

		ImGui::SameLine();
		if (ImGui::Button("Select")) {
			if (!save_file_dialog_->selected_path_.empty() && std::filesystem::is_directory(save_file_dialog_->selected_path_)) {
				save_folder_path_ = save_file_dialog_->selected_path_.generic_u8string();
			}
			changing_save_folder_ = false;
		}
	}

	ImGui::Separator();
	if (ImGui::Button("Save")) {
		Save(save_folder_path_);
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

		if (input_file_dialog_ != nullptr) {
			input_file_dialog_->width_ = window_.width_;
		}

		if (save_file_dialog_ != nullptr) {
			save_file_dialog_->width_ = window_.width_;
		}
	}
	ImGui::PopItemWidth();

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

void Application::Save(const std::string& save_folder)
{

	std::string full_path(save_folder + "/atlas.png");
	int success = stbi_write_png(full_path.c_str(), atlas_.width_, atlas_.height_, 4, (void*)atlas_.data_, atlas_.width_ * 4);

	if (!success) {
		std::cout << "Unable to save image";
	}
}

void Application::SetKeyCallbacks()
{
	glfwSetKeyCallback(window_.glfw_window_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {

		auto* app = (Application*)glfwGetWindowUserPointer(Window::glfw_window_);
	});
}

ImageData CombineAtlas(const std::vector<ImageData>& images, int size)
{

	int tex_width = size;
	int tex_height = tex_width;
	int channels = 4;
	int atlas_pitch = tex_width * channels;

	if (tex_width > 4096) {
		return ImageData();
	}

	unsigned char* pixels = (unsigned char*)calloc((size_t)tex_height * atlas_pitch, 1);
	int pen_x = 0, pen_y = 0;
	int next_pen_y = images[0].height_;
	
	for (auto& image : images) {

		int image_pitch = image.width_ * channels;
		if (pen_x + image_pitch >= atlas_pitch) {
			pen_x = 0;
			pen_y += next_pen_y;
			next_pen_y = image.height_;

			if (pen_y + image.height_ >= tex_height) {
				free(pixels);
				return ImageData();
			}
		}

		for (int row = 0; row < image.height_; ++row) {
			for (int col = 0; col < image_pitch; ++col) {
				int x = pen_x + col;
				int y = pen_y + row;
				pixels[y * (atlas_pitch) + x] = image.data_[row * (image_pitch) + col];
			}
		}

		pen_x += image_pitch;
	}
	
	return ImageData{ tex_width, tex_height, pixels };
	
}

unsigned int CreateTexture(ImageData& image)
{
	unsigned int image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Upload pixels into texture
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width_, image.height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data_);

	return image_texture;
}

void Application::UnpackFolders()
{
	std::vector<std::filesystem::path> unpacked_folders;
	for (auto& items : input_items_) {
		std::filesystem::path path = items;
		if (std::filesystem::is_directory(path)) {
			unpacked_folders.push_back(path);
		}
	}

	for (const auto& folder : unpacked_folders) {
		input_items_.erase(folder.u8string());
	}

	while (!unpacked_folders.empty()) {
		auto current_folder = unpacked_folders[0];
		unpacked_folders.erase(unpacked_folders.begin());

		for (auto& file : std::filesystem::directory_iterator(current_folder, std::filesystem::directory_options::skip_permission_denied)) {
			if (std::filesystem::is_directory(file)) {
				unpacked_folders.push_back(file.path());
			}
			else if (file.path().extension() == ".png" || file.path().extension() == ".jpg") {
				input_items_.insert(file.path().u8string());
			}
		}
	}
}

unsigned int Application::CreateAtlas(const std::unordered_set<std::string>& paths, int padding, bool pow_of_2)
{
	std::vector<ImageData> image_data = GetImageData(paths);
	std::sort(image_data.begin(), image_data.end(), [](ImageData a, ImageData b) {return a.height_ > b.height_; });
	int size = 32;
	while (atlas_.data_ == nullptr) {
		atlas_ = CombineAtlas(image_data, size);
		size *= 2;
	}
	return CreateTexture(atlas_);
}


