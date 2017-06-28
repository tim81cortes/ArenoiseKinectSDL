#include <iostream>
#include <chrono>
#include <string>
#include "App.h"

using namespace std::chrono;
typedef steady_clock Clock;



#undef main
int main(int, char**)
{



	////allocate a pixel buffer
	uint32* pixelBuffer = new uint32[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];
	if (pixelBuffer == nullptr)
		return 4;


	//printf("Pixel buffer allocated.\n");

	////clear the pixel buffer
	memset(pixelBuffer, 0, DEPTHMAPWIDTH * DEPTHMAPHEIGHT * 4);



	App app;
	app.SetPixelBuffer(pixelBuffer);
	app.Init();
	
	auto lastTime = Clock::now();

	bool running = true;
	while (running)
	{
		
		
		//calculate delta time
		const auto now = Clock::now();
		const auto duration = duration_cast<microseconds>(now - lastTime);
		const float deltaTime = duration.count() / 1000000.0f;
		lastTime = now;

		//update the application
		app.Tick(deltaTime);


	}
	



	//clean up
	app.Shutdown();

	return 0;
}

