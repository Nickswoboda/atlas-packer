# Atlas Packer
Atlas Packer is a Texture Packing application used to make sprite sheets and texture atlases for use in game development. Sumbit folders or individual .png or .jpg files and Atlas Packer will pack them into a single texture atlas. Atlas Packer can be used both with the GUI or on the command line.

## Getting Started
### Requirements
OpenGL 3.0 of higher
### Installing
##### Note: Only windows is supported at this moment.
#### Download the Executable
1. Go to the Releases section.
2. Download the AtlasPacker.zip file.
3. Extract the .zip file to your desired location.
4. Run the .exe file
#### Build with CMake
1. Go to the Releases section.
2. Download the source code.
3. `mkdir build && cd build && cmake ..`

### Usage
#### GUI
1. Input folders or individual image files using the file explorer.
2. Adjust options as needed (explained in more detail below)
3. Push 'Submit'
4. Set Save Folder location and file format.
5. Push 'Save'

#### Command Line
Usage: `AtlasPacker.exe files_or_folders... [option <arg>...]`

Example: `AtlasPacker.exe C:/Images C:/OtherImages/sprite.png -algorithm max-rects -padding 2 -force-square`
##### Note: All arguments before the first option will be considered to be an image folder or file.

##### Option List:
    --algorithm   | -a        <shelf | max-rects> [default: shelf]
    --size-solver | -ss       <fast | fixed | best-fit> [default: fast]
    --padding | -p            <NUM_PIXELS> [default: 0]
    --dimensions | -d         <WIDTH HEIGHT> [default: 4096 4096].\n\n";
    --force-square | -fs
    --power-of-two | -pot
    --output-format | -of     <png | jpg> [default: png]
    --output-directory | -od  <FOLDER> [default: executable directory]

### Options
#### Algorithm
The algorithm used to pack the atlas.

<b>- Shelf:</b> Time complexity of O(n). Very fast but not very optimal packing if used with images of varying sizes. Recommended if all images have the same dimensions.

<b>- MaxRects:</b> Time complexity of O(n<sup>3</sup>) on average and O(n<sup>5</sup>) worst case. Most optimal packing for most use cases.

#### Size Solver
Unless Fixed, the algorithm attempts to pack the images in an atlas of a minimum size. If it fails, it tries a slightly bigger atlas size until it succeeds. The size solver determines the granularity of these sizes.

<b>- Fixed:</b> Only attempts to pack an atlas of the size supplied by the user.

<b>- Fast:</b> Alternate between increasing width or height by 32 pixels until a solution is found.

<b>- Best Fit:</b> Attempts each possible dimension in order of ascending area until a solution is found. Results in the most optimal atlas size. 

#### Padding
Number of pixels between each image. Used to reduce bleeding of images when using texture mipmaps.

#### Dimensions
Size of the atlas. When size solver is Fixed, the atlas is guaranteed to be of those dimensions. Otherwise it is the maximum dimension that the atlas will attempt to pack, but may find a smaller atlas that works.

#### Force Square
Forces the width and height of the atlas to be the same. Ignored if size solver is Fixed.

#### Power of Two
Force the width and height to each be a power of two. Ignored if size solver is Fixed.

#### Output Format
File format that the atlas will be saved as. Can be either .png or .jpg.

#### Output Directory
Directory the atlas.png or atlas.jpg and metadata will be saved to. Default is the directory in which the executable is run.

### Metadata
When you save the atlas to a directory, an atlas-data.txt file will be saved along with it. This text file lists image placement data in the following format: `file path, x pos: x, y pos: y, width: w, height: h`

Example: 

        C:/images/boots.png, x pos: 168, y pos: 1001, width: 80, height: 80
        C:/images/belt.png, x pos: 877, y pos: 1144, width: 100, height: 30
        C:/images/belt2.png, x pos: 434, y pos: 1144, width: 100, height: 30

### Libraries used
- [GLAD](https://github.com/Dav1dde/glad)
- [GLFW](https://github.com/glfw/glfw)
- [ImGui](https://github.com/ocornut/imgui)
- [stb_image](https://github.com/nothings/stb)
