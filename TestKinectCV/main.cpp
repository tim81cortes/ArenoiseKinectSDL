#include <iostream>
#include <chrono>
#include <SDL.h>
#include <string>
#include "App.h"

using namespace std::chrono;
typedef steady_clock Clock;

void DrawPixelbuffer(SDL_Texture* texture, SDL_Renderer* renderer,
	const uint32* pixelBuffer)
{
 	void* pixels;
	int pitch;
	SDL_LockTexture(texture, nullptr, &pixels, &pitch);
	/*{
		printf("Could not Lock texture. \n");
		return 1;
	}*/
	if (pitch == DEPTHMAPWIDTH * 4)
		memcpy(pixels, pixelBuffer, DEPTHMAPWIDTH * DEPTHMAPHEIGHT * 4);
	else
	{
		const uint32* src = pixelBuffer;
		char* dst = (char*)pixels;
		for (int y = 0; y < DEPTHMAPHEIGHT; ++y)
		{
			memcpy(dst, src, DEPTHMAPWIDTH * 4);
			src += DEPTHMAPWIDTH;
			dst += pitch;
		}
	}
	SDL_UnlockTexture(texture);

	//copy the texture to the frame buffer
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);

	//present the frame buffer on the screen
	SDL_RenderPresent(renderer);
}

#undef main
int main(int, char**)
{
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	//initialize SDL
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_DisplayMode desktopMode;
	//SDL_GetDesktopDisplayMode(0, &desktopMode);

	//create a window
	SDL_Window *window = SDL_CreateWindow("ARENOISE_DEPTHMAP_CAPTURE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCREENWIDTH, SCREENHEIGHT, SDL_WINDOW_RESIZABLE);
	if (window == nullptr)
		return 1;

	printf("Window Opened.\n");

	//create a renderer
	SDL_Renderer* renderer = SDL_CreateRenderer(
		window, -1, SDL_RENDERER_ACCELERATED); //put back to previous SDL_RENDERER_ACCELERATED
	if (renderer == nullptr)
		return 2;
	
	//SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);


	SDL_RenderSetScale(renderer, 2.0f, 1.5f);
	SDL_RenderSetLogicalSize(renderer, DEPTHMAPWIDTH, DEPTHMAPHEIGHT);


	//printf("Worked: %s", worked);

	printf("Renderer created.\n");

	//create a texture
	SDL_Texture* texture = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING,
		DEPTHMAPWIDTH, DEPTHMAPHEIGHT); // change target to streaming to restore
	if (texture == nullptr)
		return 3;


	printf("Texture created.\n");


	//allocate a pixel buffer
	uint32* pixelBuffer = new uint32[DEPTHMAPWIDTH * DEPTHMAPHEIGHT];
	if (pixelBuffer == nullptr)
		return 4;


	printf("Pixel buffer allocated.\n");

	//clear the pixel buffer
	memset(pixelBuffer, 0, DEPTHMAPWIDTH * DEPTHMAPHEIGHT * 4);
	


	//draw pixel buffer to the screen
	DrawPixelbuffer(texture, renderer, pixelBuffer);

	printf("Pixel buffer drawn to screen.\n");

	App app;
	app.SetPixelBuffer(pixelBuffer);
	app.Init();
	
	auto lastTime = Clock::now();

	bool running = true;
	while (running)
	{
		
		if(app.getSensorPresence())
		{

			//get events
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				//pressing the cross or pressing escape will quit the application
				case SDL_QUIT:
					running = false;
					break;

				case SDL_KEYDOWN:
					if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
						running = false;
					break;

				default: //ignore other events for this demo
					break;
				}
			}

		}
		//calculate delta time
		const auto now = Clock::now();
		const auto duration = duration_cast<microseconds>(now - lastTime);
		const float deltaTime = duration.count() / 1000000.0f;
		lastTime = now;

		//update the application
		app.Tick(deltaTime);

		//draw pixel buffer to the screen
		DrawPixelbuffer(texture, renderer, pixelBuffer);
		/*{
			return 4;
		}*/
	}
	
	
	/*SDL_Rect dest_rect;
	dest_rect.h = DEPTHMAPHEIGHT;
	dest_rect.w = DEPTHMAPWIDTH;
	dest_rect.x = 0;
	dest_rect.y = 0;*/


	//SDL_FillRect(window, &dest_rect, )


	//clean up
	app.Shutdown();
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

