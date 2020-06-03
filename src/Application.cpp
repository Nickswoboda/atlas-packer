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
	:window_(width, height), input_file_dialog_(window_.width_ / 2, 250), save_file_dialog_(window_.width_ / 2, 175)
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

	save_folder_path_ = std::filesystem::current_path().u8string();
}

Application::~Application()
{
	for (int i = 0; i < MAX_IMAGES; ++i) {
		if (image_data_.data_[i] != nullptr) {
			delete image_data_.data_[i];
		}
	}
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
		ImGui::SameLine(100);
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
	window_.ResizeHeight(ImGui::GetCursorPosY());
	
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(window_.glfw_window_);
}

void Application::RenderInputState()
{
	input_file_dialog_.Render();

	//input item frame
	ImGui::SameLine();
	ImGui::BeginChildFrame(2, { window_.width_ / 2.0f, 250.0f });
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

	ImGui::SameLine((window_.width_ / 2.0f) + 10);
	if (ImGui::Button("Remove")) {
		input_items_.erase(selected_input_item);
		selected_index = -1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear All")) {
		input_items_.clear();
	}

	ImGui::Separator();
	ImGui::Separator();

	ImGui::PushItemWidth(200);
	ImGui::Text("Algorith: ");
	ImGui::SameLine(100);
	if (ImGui::BeginCombo("##Algorithm", atlas_packer_.algo_ == Algorithm::Shelf ? "Shelf" : "MaxRects")) {
		if (ImGui::Selectable("Shelf")) {
			atlas_packer_.algo_ = Algorithm::Shelf;
		}
		else if (ImGui::Selectable("MaxRects")) {
			atlas_packer_.algo_ = Algorithm::MaxRects;
		}
		ImGui::EndCombo();
	}

	ImGui::Text("Size Solver: ");
	ImGui::SameLine(100);
	static std::string combo_text = "Fast";
	if (ImGui::BeginCombo("##BoxSolver", combo_text.c_str())) {
		if (ImGui::Selectable("Fast")) {
			atlas_packer_.size_solver_ = SizeSolver::Fast;
			combo_text = "Fast";
		}
		else if (ImGui::Selectable("Best Fit")) {
			atlas_packer_.size_solver_ = SizeSolver::BestFit;
			combo_text = "Best Fit";
		}
		else if (ImGui::Selectable("Fixed")) {
			atlas_packer_.size_solver_ = SizeSolver::Fixed;
			combo_text = "Fixed";
		}
		ImGui::EndCombo();
	}

	ImGui::Text("Pixel Padding: ");
	ImGui::SameLine(100);
	if (ImGui::InputInt("##Padding", &atlas_packer_.pixel_padding_)) {
		atlas_packer_.pixel_padding_ = std::clamp(atlas_packer_.pixel_padding_, 0, 32);
	}

	if (atlas_packer_.size_solver_ == SizeSolver::Fixed) {
		ImGui::Text("Fixed Width: ");
	}
	else {
		ImGui::Text("Max Width: ");
	}
	ImGui::SameLine(100);
	if (ImGui::InputInt("##Width", &atlas_packer_.max_width_)) {
		atlas_packer_.max_width_ = std::clamp(atlas_packer_.max_width_, 0, MAX_DIMENSIONS);
	}

	if (atlas_packer_.size_solver_ == SizeSolver::Fixed) {
			ImGui::Text("Fixed Height: ");
		}
	else {

		ImGui::Text("Max Height: ");
	}
	ImGui::SameLine(100);
	if (ImGui::InputInt("##FixedHeight", &atlas_packer_.max_height_)) {
			atlas_packer_.max_height_ = std::clamp(atlas_packer_.max_height_, 0, MAX_DIMENSIONS);
	}
	

	if (!(atlas_packer_.size_solver_ == SizeSolver::Fixed)) {
		ImGui::Text("Force Square: ");
		ImGui::SameLine(100);
		ImGui::Checkbox("##ForceSquare", &atlas_packer_.force_square_);

		ImGui::Text("Power of 2: ");
		ImGui::SameLine(100);
		ImGui::Checkbox("##Powof2", &atlas_packer_.pow_of_2_);
	}

	ImGui::Separator();
	if (input_items_.empty()) {
		ImGuiErrorText("You must add an item to submit");
	}
	if (max_images_exceeded_) {
		ImGuiErrorText("The max number of images per atlas has been exceeded");
	}

	if (ImGui::Button("Submit") && !input_items_.empty()) {
		UnpackInputFolders();
		max_images_exceeded_ = unpacked_items_.size() > MAX_IMAGES;
		if (max_images_exceeded_) {
			unpacked_items_.clear();
			return;
		}
		if (!unpacked_items_.empty()) {
			GetImageData(unpacked_items_, image_data_);
			//returns index of image_data_ that the atlas image data resides. equivalent to image_data_.num_images
			atlas_index_ = atlas_packer_.CreateAtlas(image_data_);

			//used to display preview in output window
			atlas_texture_ID_ = CreateAtlasTexture(atlas_index_);


			if (image_data_.rects_[atlas_index_].w <= 512 && image_data_.rects_[atlas_index_].h <= 512) {
				preview_size_ = { image_data_.rects_[atlas_index_].w, image_data_.rects_[atlas_index_].h };
			}
			else {
				float aspect_ratio = image_data_.rects_[atlas_index_].w / (float)image_data_.rects_[atlas_index_].h;
				preview_size_ = aspect_ratio < 1 ? Vec2{ (int)(512 * aspect_ratio), 512 } : Vec2{ 512, (int)(512 * (1 / aspect_ratio)) };
			}

			PushState(State::Output);
		}
	}
}

void Application::RenderOutputState()
{
	ImGui::Text("Preview");

	if (atlas_texture_ID_ == -1 || atlas_index_ == -1) {
		ImGuiErrorText("Unable to create atlas");
	}
	else {

		ImGui::Text("Width: %i, Height: %i", image_data_.rects_[atlas_index_].w, image_data_.rects_[atlas_index_].h);

		ImGui::Image((void*)(intptr_t)atlas_texture_ID_, { (float)preview_size_.x, (float)preview_size_.y }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, { 1,1,1,1 });


		ImGui::Separator();
		ImGui::Text("Stats:");
		ImGui::Text("Unused area: %i px", atlas_packer_.stats_.unused_area);
		ImGui::Text("Packing efficiency: %.2f%%", atlas_packer_.stats_.packing_efficiency);
		ImGui::Text("Time to pack: %.2f ms", atlas_packer_.stats_.time_elapsed_in_ms);
	}

	ImGui::PushItemWidth(200);

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

	ImGui::Text("Save File Format:"); ImGui::SameLine(120);
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

	if (save_folder_path_.empty()) {
		ImGuiErrorText("You must choose a save destination folder");
	}
	if (ImGui::Button("Save") && !save_folder_path_.empty()) {
		Save(save_folder_path_);
	}
	ImGui::Separator();

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
		success = stbi_write_png(full_path.c_str(), image_data_.rects_[atlas_index_].w, image_data_.rects_[atlas_index_].h, 4, (void*)image_data_.data_[atlas_index_], image_data_.rects_[atlas_index_].w * 4);
	}
	else {
		std::string full_path(save_folder + "/atlas.jpg");
		success = stbi_write_jpg(full_path.c_str(), image_data_.rects_[atlas_index_].w, image_data_.rects_[atlas_index_].h, 4, (void*)image_data_.data_[atlas_index_], jpg_quality_);
	}
	if (!success) {
		std::cout << "Unable to save image";
		return;
	}

	std::ofstream file(save_folder + "/atlas-data.txt");
	file << atlas_packer_.metadata_ << std::endl;
}

unsigned int Application::CreateAtlasTexture(int image_index)
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_data_.rects_[image_index].w, image_data_.rects_[image_index].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data_.data_[image_index]);

	return image_texture;
}

void Application::CreateAtlasFromCmdLine(int argc, char** argv)
{
	int index = 1;
	while (index < argc && argv[index][0] != '-') {
		input_items_.insert(argv[index]);
		++index;
	}

	while (index < argc) {
		std::string option = argv[index];
		if (option == "-a" || option == "--algorithm") {
			if (index + 1 >= argc) {
				std::cout << "No arguments have been provided for " << option << "\n";
				return;
			}
			std::string arg = argv[index + 1];
			if (arg == "max-rects") {
				atlas_packer_.algo_ = Algorithm::MaxRects;
			}
			//Shelf is default so no need to set
			else if (arg != "shelf") {
				std::cout << arg << " is not a valid algorithm\n";
				return;
			}
			//additonal increment to go past arg and get to next option
			++index;
		}
		else if (option == "-ss" || option == "--size-solver") {
			if (index + 1 >= argc) {
				std::cout << "No arguments have been provided for " << option << ".\n";
				return;
			}
			std::string arg = argv[index + 1];
			if (arg == "fixed") {
				atlas_packer_.size_solver_ = SizeSolver::Fixed;
			}
			else if (arg == "fast") {
				atlas_packer_.size_solver_ = SizeSolver::Fast;
			}
			else if (arg == "best-fit") {
				atlas_packer_.size_solver_ = SizeSolver::BestFit;
			}
			else {
				std::cout << arg << " is not a valid size solver.\n";
				return;
			}
			++index;
		}
		else if (option == "-p" || option == "--padding") {
			if (index + 1 >= argc) {
				std::cout << "No arguments have been provided for " << option << ".\n";
				return;
			}
			if (!IsNumber(argv[index + 1])) {
				std::cout << argv[index + 1] << " is not a valid number.\n";
				return;
			}
			int padding = std::stoi(argv[index + 1]);
			if (padding > 32) {
				std::cout << "The maximum pixel padding allowed is 32.\n";
				return;
			}
			atlas_packer_.pixel_padding_ = padding;
			++index;
		}
		else if (option == "-d" || option == "--dimensions") {
			if (index + 1 >= argc || index + 2 >= argc) {
				std::cout << "No arguments have been provided for " << option << ".\n";
				return;
			}
			if (!IsNumber(argv[index + 1]) || !IsNumber(argv[index + 2])) {
				std::cout << argv[index + 1] << "x" << argv[index+2] << " is not a valid size.\n";
				return;
			}

			int width = std::stoi(argv[index + 1]);
			int height = std::stoi(argv[index + 2]);
			if (width > 4096 || height > 4096) {
				std::cout << "The maximum dimensions are 4096x4096.\n";
				return;
			}

			atlas_packer_.max_width_ = width;
			atlas_packer_.max_height_ = height;
			//additonal increment to use up both arg
			index +=2 ;
		}
		else if (option == "-fs" || option == "--force-square") {
			atlas_packer_.force_square_ = true;
			if (atlas_packer_.size_solver_ == SizeSolver::Fixed) {
				std::cout << "Forced Square is ignored for fixed size atlases.\n";
			}
		}
		else if (option == "-pot" || option == "--power-of-two") {
			atlas_packer_.pow_of_2_ = true;
			if (atlas_packer_.size_solver_ == SizeSolver::Fixed) {
				std::cout << "Power of 2 is ignored for fixed size atlases.\n";
			}
		}
		else if (option == "-of" || option == "--output-format") {
			if (index + 1 >= argc) {
				std::cout << "No arguments have been provided for " << option << ".\n";
				return;
			}
			std::string arg = argv[index + 1];
			if (arg == "jpg") {
				save_file_format_ = SaveFileFormat::JPG;
			}
			//png is default
			else if (arg != "png") {
				std::cout << arg << " is not a valid file format.\n";
			}

		}
		else if (option == "-od" || option == "--output-directory") {
			if (index + 1 >= argc) {
				std::cout << "No arguments have been provided for " << option << ".\n";
				return;
			}
			std::filesystem::path dir(argv[index + 1]);
			if (std::filesystem::is_directory(dir)){
				save_folder_path_ = dir.generic_u8string();
			}
			else {
				std::cout << argv[index + 1] << "is not a valid directory.\n";
			}
			++index;
		}
		else {
			std::cout << option << " is not a valid option.";
		}

		++index;
	}

	//if exited out of parsing early due to errors
	if (index != argc) {
		std::cout << "Try 'AtlasPacker.exe --help' for more information.\n";
	}

	UnpackInputFolders();
	if (unpacked_items_.empty()) {
		std::cout << "No valid textures found.\n";
		std::cout << "Try 'AtlasPacker.exe --help' for more information.\n";
		return;
	}

	GetImageData(unpacked_items_, image_data_);
	atlas_index_ = atlas_packer_.CreateAtlas(image_data_);
	Save(save_folder_path_);

	if (atlas_index_ == -1) {
		std::cout << "Unable to create atlas with the current settings. Please try again.\n";
		return;
	}

	std::cout << "Atlas creation complete.\n" <<
		"Time to pack: " << atlas_packer_.stats_.time_elapsed_in_ms << "ms\n" <<
		"Unused area:  " << atlas_packer_.stats_.unused_area << "px\n" <<
		"Packing efficiency: " << std::fixed << std::setprecision(2) << atlas_packer_.stats_.packing_efficiency << "%\n";
	std::cout << "Atlas saved to " << save_folder_path_ << ".\n";
}

bool Application::IsNumber(const std::string& value)
{
	for (const auto& c : value) {
		if (!std::isdigit(c)) {
			return false;
		}
	}

	return true;
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


