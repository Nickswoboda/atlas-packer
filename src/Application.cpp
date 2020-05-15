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
	if (ImGui::InputInt("##Padding", &atlas_packer_.pixel_padding_)) {
		if (atlas_packer_.pixel_padding_ < 0) {
			atlas_packer_.pixel_padding_ = 0;
		}
		if (atlas_packer_.pixel_padding_ > 32) {
			atlas_packer_.pixel_padding_ = 32;
		}
	}
	ImGui::Text("Max Width: ");
	ImGui::SameLine();
	if (ImGui::InputInt("##MaxWidth", &atlas_packer_.max_width_)) {
		if (atlas_packer_.max_width_ < 0) {
			atlas_packer_.max_width_ = 0;
		}
		if (atlas_packer_.max_width_ > 4096) {
			atlas_packer_.max_width_ = 4096;
		}
	}

	ImGui::Text("Max Height: ");
	ImGui::SameLine();
	if (ImGui::InputInt("##MaxHeight", &atlas_packer_.max_height_)) {
		if (atlas_packer_.max_height_ < 0) {
			atlas_packer_.max_height_ = 0;
		}
		if (atlas_packer_.max_height_ > 4096) {
			atlas_packer_.max_height_ = 4096;
		}
	}

	ImGui::Text("Power of 2: ");
	ImGui::SameLine();
	ImGui::Checkbox("##Powof2", &atlas_packer_.pow_of_2_);

	if (input_items_.empty()) {
		ImGuiErrorText("You must add an item to submit");
	}
	if (ImGui::Button("Submit") && !input_items_.empty()) {
		UnpackInputFolders();
		if (!unpacked_items_.empty()) {
			GetImageData(unpacked_items_, image_data_);
			atlas_index_ = atlas_packer_.CreateAtlas(image_data_);
			atlas_texture_ID_ = CreateTexture(atlas_index_);
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
		int aspect_ratio = image_data_.sizes_[atlas_index_].x / image_data_.sizes_[atlas_index_].y;
		ImGui::Image((void*)(intptr_t)atlas_texture_ID_, { 256.0f, 256.0f }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, { 1,1,1,1 });

		ImGui::Text("Stats:");
		ImGui::Text("Unused area: %i px", atlas_packer_.stats_.unused_area);
		ImGui::Text("Packing efficiency: %.2f%%", atlas_packer_.stats_.packing_efficiency);
		ImGui::Text("Time to pack: %.2f ms", atlas_packer_.stats_.time_elapsed_in_ms);
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
		success = stbi_write_png(full_path.c_str(), image_data_.sizes_[atlas_index_].x, image_data_.sizes_[atlas_index_].y, 4, (void*)image_data_.data_[atlas_index_], image_data_.sizes_[atlas_index_].x * 4);
	}
	else {
		std::string full_path(save_folder + "/atlas.jpg");
		success = stbi_write_jpg(full_path.c_str(), image_data_.sizes_[atlas_index_].x, image_data_.sizes_[atlas_index_].y, 4, (void*)image_data_.data_[atlas_index_], jpg_quality_);
	}
	if (!success) {
		std::cout << "Unable to save image";
	}

	std::ofstream file(save_folder + "/atlas-data.json");
	file << std::setw(4) << atlas_packer_.data_json_ << std::endl;
}

unsigned int Application::CreateTexture(int image_index)
{
	if (image_data_.data_[image_index] == nullptr) {
		return -1;
	}
	unsigned int image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_data_.sizes_[image_index].x, image_data_.sizes_[image_index].y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data_.data_[image_index]);

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
				unpacked_items_.push_back(file.path().u8string());
			}
		}

		++num_processed_folders;
	}
}


