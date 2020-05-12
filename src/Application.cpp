#include "Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

#include <iostream>
#include <filesystem>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


Application::Application(int width, int height)
	:window_(width, height), input_file_dialog_(window_.width_ / 2, window_.height_ / 3), save_file_dialog_(window_.width_ / 2, window_.height_ / 3)
{
	if (!gladLoadGLLoader(GLADloadproc(glfwGetProcAddress))) {
		std::cout << "could not load GLAD";
	}

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
		glfwPollEvents();

		if (Window::IsFocused()) {
			Render();
		}

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

	//move base transparent window when moving ImGui window
	ImVec2 pos = ImGui::GetWindowPos();
	if (pos.x != 0.0f || pos.y != 0.0f) {
		ImGui::SetWindowPos({ 0.0f, 0.0f });
		window_.Move(pos.x, pos.y);
	}

	//resize height based on amount of widgets on screen
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
	input_file_dialog_.Render();

	ImGui::SameLine();
	ImGui::BeginChildFrame(2, { window_.width_ / 2.0f, window_.height_ / 3.0f });
	int index = 0;
	static int selected_index = -1;
	static std::string selected_input_item;
	for (auto& item : input_items_) {

		if (ImGui::Selectable(item.c_str(), selected_index == index)) {
			selected_index = index;
			selected_input_item = item.c_str();
		}
		++index;
	}
	ImGui::EndChildFrame();

	if (ImGui::Button("Prev")) {
		input_file_dialog_.GoBack();
	}

	ImGui::SameLine();
	if (ImGui::Button("Add")) {
		if (!input_file_dialog_.selected_path_.empty()) {
			input_items_.insert(input_file_dialog_.selected_path_);
		}
	}

	ImGui::SameLine(window_.width_ / 2.0f);
	if (ImGui::Button("Remove")) {
		input_items_.erase(selected_input_item);
		selected_index = -1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear All")) {
		input_items_.clear();
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

	if (input_items_.empty()) {
		ImGuiErrorText("You must add an item to submit");
	}
	if (ImGui::Button("Submit") && !input_items_.empty()) {
		UnpackInputFolders();
		if (!unpacked_items_.empty()) {
			atlas_texture_ID_ = CreateAtlas(unpacked_items_, pixel_padding_, pow_of_2_);
			PushState(State::Output);
		}
	}

}

void Application::RenderOutputState()
{
	ImGui::Text("Preview");

	if (atlas_texture_ID_ == -1) {
		ImGuiErrorText("Unable to create atlas");
	}
	else {
		int aspect_ratio = atlas_.width_ / atlas_.height_;
		ImGui::Image((void*)(intptr_t)atlas_texture_ID_, { (float)256 * aspect_ratio, 256 }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, { 1,1,1,1 });
	}

	ImGui::Separator();
	ImGui::Text("Save Folder Path: %s", save_folder_path_.c_str());
	ImGui::SameLine();
	if (ImGui::Button("Change")){
		changing_save_folder_ = true;
	}

	if (changing_save_folder_) {
		save_file_dialog_.Render();

		if (ImGui::Button("Prev")) {
			save_file_dialog_.GoBack();
		}

		ImGui::SameLine();
		if (ImGui::Button("Select")) {
			if (std::filesystem::is_directory(save_file_dialog_.selected_path_)) {
				save_folder_path_ = save_file_dialog_.selected_path_;
				changing_save_folder_ = false;
			}
		}
	}

	ImGui::Text("Save File Format:"); ImGui::SameLine();
	if (ImGui::BeginCombo("##SaveFormat", save_file_format_ == SaveFileFormat::PNG ? ".png" : ".jpg")) {
		if (ImGui::Selectable(".png")) {
			save_file_format_ = SaveFileFormat::PNG;
		}
		else if (ImGui::Selectable(".jpg")) {
			save_file_format_ = SaveFileFormat::JPG;
		}

		ImGui::EndCombo();
	}

	if (save_file_format_ == SaveFileFormat::JPG) {
		ImGui::Text("JPG quality level: "); ImGui::SameLine();
		ImGui::SliderInt("##jpgquality", &jpg_quality_, 1, 100);
	}

	ImGui::Separator();
	if (save_folder_path_.empty()) {
		ImGuiErrorText("You must choose a save destination folder");
	}
	if (ImGui::Button("Save") && !save_folder_path_.empty()) {
		Save(save_folder_path_);
	}

	if (ImGui::Button("Try Again")) {
		unpacked_items_.clear();
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
	if (ImGui::DragInt("##width", &window_.width_, 1.0f, 100, 1024)) {
		window_.UpdateSize();
		input_file_dialog_.width_ = window_.width_ / 2;
		save_file_dialog_.width_ = window_.width_ / 2;
	}
	ImGui::PopItemWidth();

	if (ImGui::Button("Return")) {
		PopState();
	}
}

void Application::ImGuiErrorText(const std::string& error)
{
	ImGui::PushStyleColor(ImGuiCol_Text, { 1.0, 0.0, 0.0, 1.0 });
	ImGui::Text(error.c_str());
	ImGui::PopStyleColor();
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
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
}

void Application::Save(const std::string& save_folder)
{
	int success;

	if (save_file_format_ == SaveFileFormat::PNG) {
		std::string full_path(save_folder + "/atlas.png");
		success = stbi_write_png(full_path.c_str(), atlas_.width_, atlas_.height_, 4, (void*)atlas_.data_, atlas_.width_ * 4);
	}
	else {
		std::string full_path(save_folder + "/atlas.jpg");
		success = stbi_write_jpg(full_path.c_str(), atlas_.width_, atlas_.height_, 4, (void*)atlas_.data_, jpg_quality_);
	}
	if (!success) {
		std::cout << "Unable to save image";
	}

	std::ofstream file(save_folder + "/atlas-data.json");
	file << std::setw(4) << output_json_ << std::endl;
}

ImageData CombineAtlas(const std::vector<ImageData>& images, std::unordered_map<std::string, Vec2> placement, int width, int height)
{
	int channels = 4;
	int atlas_pitch = width * channels;

	unsigned char* pixels = new unsigned char[(size_t)height * atlas_pitch]();
	
	nlohmann::json json;
	for (auto& image : images) {

		int image_pitch = image.width_ * channels;

		int pen_x = placement[image.path_name].x * channels;
		int pen_y = placement[image.path_name].y;

		for (int row = 0; row < image.height_; ++row) {
			for (int col = 0; col < image_pitch; ++col) {
				int x = pen_x + col;
				int y = pen_y + row;
				pixels[y * (atlas_pitch) + x] = image.data_[row * (image_pitch) + col];
			}
		}
	}

	return ImageData{ "atlas", width, height, pixels };
}

unsigned int CreateTexture(ImageData& image)
{
	unsigned int image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width_, image.height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data_);

	return image_texture;
}

void Application::UnpackInputFolders()
{
	std::vector<std::filesystem::path> unpacked_folders;

	for (auto& items : input_items_) {
		std::filesystem::path path = items;
		if (std::filesystem::is_directory(path)) {
			unpacked_folders.push_back(path);
		}
	}

	static int max_processed_folders = 20;
	int num_processed_folders = 0;
	while (!unpacked_folders.empty() && num_processed_folders <= max_processed_folders) {
		auto current_folder = unpacked_folders[0];
		unpacked_folders.erase(unpacked_folders.begin());

		for (auto& file : std::filesystem::directory_iterator(current_folder, std::filesystem::directory_options::skip_permission_denied)) {
			if (file.is_directory()) {
				unpacked_folders.push_back(file.path());
			}
			else if (file.path().extension() == ".png" || file.path().extension() == ".jpg") {
				unpacked_items_.insert(file.path().u8string());
			}
		}

		++num_processed_folders;
	}
}

unsigned int Application::CreateAtlas(const std::unordered_set<std::string>& paths, int padding, bool pow_of_2)
{
	std::vector<ImageData> image_data = GetImageData(paths);
	std::sort(image_data.begin(), image_data.end(), [](ImageData a, ImageData b) {return a.height_ > b.height_; });

	int total_area = 0;
	for (const auto& image : image_data) {
		total_area += image.width_ * image.height_;
	}

	int atlas_width = 16;
	int atlas_height = 16;
	while (atlas_width * atlas_height < total_area) {
		atlas_width == atlas_height ? atlas_width *= 2 : atlas_height = atlas_width;
	}

	std::unordered_map<std::string, Vec2> placement;
	while (placement.empty()) {
		if (atlas_width > 4096) {
			return -1;
		}
		std::cout << "Trying: " << atlas_width << ", " << atlas_height << "\n";

		placement = GetTexturePlacements(image_data, atlas_width, atlas_height, padding);
		
		if (!placement.empty()) break;
		atlas_width == atlas_height ? atlas_width *= 2 : atlas_height = atlas_width;
	}
	atlas_ = CombineAtlas(image_data, placement, atlas_width, atlas_height);
	output_json_ = CreateJsonFile(image_data, placement);
	return CreateTexture(atlas_);
}

nlohmann::json Application::CreateJsonFile(const std::vector<ImageData>& images, std::unordered_map<std::string, Vec2> placement)
{
	nlohmann::json json;

	for (const auto& image : images) {
		json[image.path_name]["x_pos"] = placement[image.path_name].x;
		json[image.path_name]["y_pos"] = placement[image.path_name].y;
		json[image.path_name]["width"] = image.width_;
		json[image.path_name]["height"] = image.height_;
	}

	return json;
}

std::unordered_map<std::string, Vec2> Application::GetTexturePlacements(const std::vector<ImageData>& images, int width, int height, int padding)
{
	std::unordered_map<std::string, Vec2> placement;

	int pen_x = 0, pen_y = 0;
	int next_pen_y = images[0].height_;

	for (auto& image : images) {

		while (pen_x + image.width_ >= width) {
			pen_x = 0;
			pen_y += next_pen_y;
			next_pen_y = image.height_;

			if (pen_y + image.height_ >= height) {
				return std::unordered_map<std::string, Vec2>();
			}
		}

		placement[image.path_name] = { pen_x, pen_y };

		pen_x += image.width_;
	}

	return placement;
}


