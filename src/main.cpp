#include "Application.h"

#include <iostream>
void OutputHelpMessage()
{
	std::string help = "Usage: AtlasPacker.exe files_or_folders... [option <arg>...]\n";
	help += "Example: AtlasPacker.exe C:/Images C:/OtherImages/sprite.png -algorithm max-rects -padding 2 -force-square\n\n";
	help += "All arguments before the first option will be considered to be an image folder/file.\n\n";

	help += "Option List:\n";
	help += "--algorithm | -a  <shelf | max-rects>\t\tAlgorithm used to Pack Atlas [default: shelf].\n\n";
	help += "--size-solver | -ss  <fast | fixed | best-fit>\tSize Solver used to determine size of the atlas [default: fast].\n\n";

	help += "--padding | -p  <NUM_PIXELS>\t\t\tPadding of NUM_PIXELS is applied between each image. Max: 32 [default: 0].\n\n";

	help += "--dimensions | -d  <WIDTH HEIGHT>\t\tSets the maximum dimensions to WIDTH and HEIGHT respectively. Max: 4096 4096.\n";
	help += "\t\t\t\t\t\tIf Size Solver is Fixed then is used as the fixed size [default: 4096 4096].\n\n";

	help += "--force-square | -fs\t\t\t\tForces atlas to have the same width and height. Ignored if Size Solver is Fixed.\n\n";

	help += "--power-of-two | -pot\t\t\t\tForces atlas to have power of two dimensions. Ignored if Size Solver is Fixed.\n\n";

	help += "--output-format | -of  <png | jpg>\t\tSets the file format of the atlas [default: png].\n\n";

	help += "--output-directory | -od  <FOLDER>\t\tSets the output directory of the atlas to FOLDER [default: executable directory].\n\n";
	
	std::cout << help;
}

int main(int argc, char* argv[])
{
	Application app(520, 720);

	if (argc > 1) {
		std::string first_cmd = argv[1];
		if (first_cmd == "--help") {
			OutputHelpMessage();
		}
		else {
			app.CreateAtlasFromCmdLine(argc, argv);
		}
	}
	else {
		app.Run();
	}

	return 0;

}