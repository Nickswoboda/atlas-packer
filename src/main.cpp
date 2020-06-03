#include "Application.h"

#include <iostream>
void OutputHelpMessage()
{
	std::cout << "Usage: AtlasPacker.exe <Image Files/Folder> <cmd> <args>";

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