cmake_minimum_required (VERSION 3.8)

set (CMAKE_CXX_STANDARD 17)

project ("Atlas")

add_executable (Atlas
	"src/main.cpp"
	"src/Application.cpp"
	"src/Window.cpp"
	"src/FileDialog.cpp"
	"dependencies/GLAD/src/glad.c"
	"dependencies/imgui/imgui.cpp"
	"dependencies/imgui/imgui_draw.cpp"
	"dependencies/imgui/imgui_demo.cpp"
	"dependencies/imgui/imgui_impl_glfw.cpp"
	"dependencies/imgui/imgui_impl_opengl3.cpp"
	"dependencies/imgui/imgui_widgets.cpp"
 "src/ImageData.cpp" "src/AtlasPacker.cpp")

target_include_directories(Atlas
PUBLIC
	"dependencies/GLFW/include"
	"dependencies/GLAD/include"
	"dependencies/stb_image"
	"dependencies/imgui")

add_subdirectory("dependencies/GLFW")

target_link_libraries(Atlas glfw)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})