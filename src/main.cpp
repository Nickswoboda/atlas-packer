#include "Application.h"

int main(int argc, char* argv[])
{
	Application app(520, 720);

	//constexpr int argc1 = 17;
	//char* argv1[argc1];
	//argv1[0] = "Atlas.exe";
	//argv1[1] = "C:/images";
	//argv1[2] = "-a";
	//argv1[3] = "max-rects";
	//argv1[4] = "-ss";
	//argv1[5] = "fixed";
	//argv1[6] = "-p";
	//argv1[7] = "32";
	//argv1[8] = "-d";
	//argv1[9] = "900";
	//argv1[10] = "900";
	//argv1[11] = "-fs";
	//argv1[12] = "-pot";
	//argv1[13] = "-of";
	//argv1[14] = "png";
	//argv1[15] = "-od";
	//argv1[16] = "C:/dev";
	
	//if (true) {
	//	app.CreateAtlasFromCmdLine(argc1, argv1);
	//}
	if (argc > 1) {
		app.CreateAtlasFromCmdLine(argc, argv);
	}
	else {
		app.Run();
	}

	return 0;

}