cmake_minimum_required (VERSION 3.8)

set (CMAKE_CXX_STANDARD 17)

project ("Atlas")

IF(WIN32)
IF(CMAKE_BUILD_TYPE MATCHES Release)
set(CMAKE_CREATE_WIN32_EXE "/SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
ENDIF(CMAKE_BUILD_TYPE MATCHES Release)
ENDIF(WIN32)

set (src 	"src/main.cpp"
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

IF(WIN32)
IF(CMAKE_BUILD_TYPE MATCHES Release)
add_executable (Atlas WIN32
	${src})
ELSE()
add_executable (Atlas
	${src})
ENDIF(CMAKE_BUILD_TYPE MATCHES Release)
ENDIF(WIN32)

target_include_directories(Atlas
PUBLIC
	"dependencies/GLFW/include"
	"dependencies/GLAD/include"
	"dependencies/stb_image"
	"dependencies/imgui")

add_subdirectory("dependencies/GLFW")

target_link_libraries(Atlas glfw)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})