#include "Application.h"

int main(int argc, char* argv[])
{
	Application app(520, 720);

	if (argc > 1) {
		app.CreateAtlasFromCmdLine(argc, argv);
	}
	else {
		app.Run();
	}

	return 0;

}