# Atlas Packer
Atlas Packer is a Texture Packing application used to make sprite sheets and texture atlases for use in game development. Sumbit folders or individual .png or .jpg files and Atlas Packer will pack them into a single texture atlas. Atlas Packer can be used both with the GUI or on the command line.

## Getting Started
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
##### Algorithm
The algorithm used to pack the atlas.

<b>Shelf:</b> Time complexity of O(n). Very fast but not very optimal packing if used with images of varying sizes. Recommended if all images have the dimensions.

<b>MaxRects:</b> Time complexity of O(n<sup>3</sup>) on average and O(n<sup>5</sup>) worst case. Most optimal packing for most use cases.

##### Size Solver
Unless Fixed, the algorithm attempts to pack the images in an atlas of a minimum size. If it fails, it tries a slightly bigger atlas size until it succeeds. The size solver determines the granularity of these sizes.

<b>Fixed:</b>Only attempts to pack an atlas of the size supplied by the user.

<b>Fast:</b> Alternate between increasing width or height by 32 pixels until a solution is found.

<b>Best Fit:</b> Attempts each possible dimension in order of ascending area until a solution is found. Results in the most optimal atlas size. 
